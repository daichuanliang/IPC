//g++ -std=c++11 ipc.cpp -lopencv_core -lopencv_imgproc -lopencv_highgui -pthread
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <atomic>
#include <chrono>

#ifndef USE_BOOST
#include <regex>
#else
#include <boost/regex.hpp>
#endif

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

#include "face_recog/age_gender_classify.h"
#include "face_recog/face_recognition.h"

#include "meta.h"

#include <X11/Xlib.h>

//using namespace tensorflow;

#include "Config.h"

#ifdef ENABLE_JSON_POST
#include "jsonAPI.h"
#include "curlPost.h"
#endif

using namespace cv;
using namespace std;
using namespace rapidjson;
using namespace Eigen;
using namespace chrono;

#define MAXLINE 1024
#define DEBUG true
#define BUF_LEN_JSON (4096)

const char STR_IPDOMAIN[]="IPDomain";
const char STR_MACTABLE[]="MacTable";
const char STR_MAC[]="mac";
const char STR_VENDOR[]="vendor";
const char STR_USER[]="user";
const char STR_PASSWD[]="passwd";

mutex m;

// CascadeClassifier face_cascade;
std::vector<Eigen::ArrayXf> global_id_dataset ;
std::vector<string> id_name;
vector<Meta_t> detectAndGuess(Mat frame, std::unique_ptr<tensorflow::Session> *session ,CascadeClassifier face_cascade);

float arrayXdDistance(const Ref<const ArrayXf> x, const Ref<const ArrayXf> y)
{
    return (double)sqrt((x-y).square().sum());
}

int parse_json(const char* jsonfile, string& IPDomain, vector<tuple<string,string,string,string>>& mac_table){
	FILE *fp = fopen(jsonfile , "rb");
	if (fp == nullptr){
		cerr<<"Fail to find the json config file"<<endl;
		return -1;
	}
	char readbuffer[BUF_LEN_JSON];
	FileReadStream frs(fp , readbuffer , sizeof(readbuffer));
	Document doc;
	doc.ParseStream(frs);
	fclose(fp);

	assert(!doc.HasParseError());

	if (doc.HasMember(STR_IPDOMAIN)){
		cout<<"found IPDomain entry"<<endl;

		//提取数组元素（声明的变量必须为引用）
		Value &val = doc[STR_IPDOMAIN];
		assert(val.IsString());
		IPDomain = val.GetString();
		cout<<"IPDomain from json: :" << IPDomain <<endl;
	}

	Value &vs = doc[STR_MACTABLE];
	assert(vs.IsArray());

	cout<<"parse result:"<<endl;
	for (auto i = 0; i<vs.Size(); i++)
	{
		//逐个提取数组元素（声明的变量必须为引用）
		Value &v = vs[i];
		assert(v.IsObject());
		assert(v.HasMember(STR_MAC));
		string mac = v[STR_MAC].GetString();

		string vendor = "hc"; //haikang by default
		if (v.HasMember(STR_VENDOR)){
			vendor = v[STR_VENDOR].GetString();
		}
		string user = "admin"; //haikang by default
		if (v.HasMember(STR_USER)){
			user = v[STR_USER].GetString();
		}
		string passwd = "passwd"; //haikang by default
		if (v.HasMember(STR_PASSWD)){
			passwd = v[STR_PASSWD].GetString();
		}

		cout<<"mac:"<<mac<<"  vendor:"<<vendor<<"  user:"<<user<<"  passwd:"<<passwd<<endl;
		mac_table.push_back(make_tuple(mac, vendor,user,passwd));
	}
	return 0;
}

int main(int argc, char** argv)
{
    bool is_gui_present = true;
    if (NULL == getenv("DISPLAY"))
        is_gui_present = false;
    if (is_gui_present)
        XInitThreads();

	string IPDomain;
	vector<tuple<string,string,string,string>> mac_table; //mac, vendor, user, passwd
#if 0
	mac_table.push_back(make_tuple("b4:a3:82:5e:5f:15", "hc", "admin", "xxx"));
	mac_table.push_back(make_tuple("4c:bd:8f:c6:30:1f", "hc", "admin", "xxx"));
	mac_table.push_back(make_tuple("4c:bd:8f:3f:19:db", "hc", "admin", "xxx"));
	mac_table.push_back(make_tuple("b4:a3:82:66:8b:e6", "hc", "admin", "xxx"));
	mac_table.push_back(make_tuple("b4:a3:82:6a:80:f0", "hc", "admin", "xxx"));
	mac_table.push_back(make_tuple("14:a7:8b:8f:d8:37", "dh", "admin", "xxx"));
	IPDomain = "192.168.0.0/24";
#endif

	if(parse_json("cfg.json", IPDomain, mac_table)){
		cerr<<"Failed to parse cfg.json"<<endl;
		return -1;
	}


	char result_buf[MAXLINE];
	vector<tuple<string,string,string,string,string>> mac_ip_vendor_table;

	cout<<endl<<"Start to scan local device ip-mac tables....."<<endl;

#if 1
	string nmapcmd = string("sudo nmap -sP ") + IPDomain + " | awk '/Nmap scan report for/{printf $5;}/MAC Address:/{print \" => \"$3;}' | sort";
	FILE* fp = popen(nmapcmd.c_str(), "r");
	//FILE* fp = popen("sudo nmap -sP  192.168.0.0/24 | awk '/Nmap scan report for/{printf $5;}/MAC Address:/{print \" => \"$3;}' | sort", "r");
	if(NULL == fp)
	{
		cerr<<"fail to execute arp -a"<<endl;
		exit(1);
	}
	while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
	{
		int len=strlen(result_buf);
		if (result_buf[len-1] == '\n'){
			result_buf[len-1]=0;
		}
		if (DEBUG){
			cout<<"Try search arp result: "<<result_buf<<endl;
		}
		string str=result_buf;
		transform(str.begin(), str.end(), str.begin(), ::tolower);

		for (auto macvendor:mac_table){
			string mac;
			string vendor;
			string user;
			string passwd;
			tie(mac,vendor,user, passwd)=macvendor;
#ifndef USE_BOOST
			regex re(string("([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*=>.*")+mac+".*");
			smatch sm;
			regex_match(str, sm, re);
			if (DEBUG){
				for (auto s:sm){
					cout<<"sm: " << s<<endl;
				}
			}
			if (sm.size() == 2){
				cout<<"ip="<<sm[1]<<" mac="<<mac<<endl;
				mac_ip_vendor_table.push_back(tuple<string,string,string,string,string>(sm[1], mac, vendor, user, passwd));
			}
#else
			string mas = string("([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*=>.*")+mac+".*";
			boost::regex re(mas);
			boost::smatch sm;
			bool match_ret = boost::regex_match(str, sm, re);
			if (match_ret && sm.size() == 2){
				cout<<"Match: " << "ip="<<sm[1]<<" mac="<<mac<<endl;
				mac_ip_vendor_table.push_back(tuple<string,string,string,string,string>(sm[1], mac, vendor, user, passwd));
			}
#endif
		}
	}
#else
	mac_ip_table.push_back(make_tuple("192.168.0.106","b4:a3:82:5e:5f:15","hc"));
	mac_ip_table.push_back(make_tuple("192.168.0.108","b4:a3:82:5e:5f:15","hc"));
	mac_ip_table.push_back(make_tuple("192.168.0.109","4c:bd:8f:c6:30:1f","hc"));
	mac_ip_table.push_back(make_tuple("192.168.0.105","33:bd:8f:c6:30:1f","hc"));
#endif

	cout<<"Scan Done."<<endl<<endl;

	cout<<"Found ip - map mapping:"<<endl;
	for (auto s:mac_ip_vendor_table){
		string ip;
		string mac;
		string vendor;
		string user;
		string passwd;
		tie(ip,mac,vendor,user, passwd)=s;
		cout<<"    "<<ip<<" => "<<mac<<" => "<<vendor<<" => "<<user<<" => "<<passwd<<endl;
	}
	cout<<endl;

	//start config

#ifdef RESET_CONFIG
	{
		Config cfg;
		for (auto s:mac_ip_vendor_table){
			string ip;
			string mac;
			string vendor;
			string user;
			string passwd;
			tie(ip,mac,vendor,user, passwd)=s;
			if (vendor == "hc")
				cfg.updateConfig(ip, 8000, user, passwd);

		}
	}
#endif
    // String face_cascade_name = "haarcascade_frontalface_alt.xml";
    // if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading face cascade\n"); return -1; };

	//start rtsp

	vector<thread> ts;
	for (auto s:mac_ip_vendor_table){
		string ip;
		string mac;
		string vendor;
		string user;
		string passwd;
		tie(ip,mac,vendor,user, passwd)=s;

		string rtsp;
		if (vendor == string("dh")){
			cout<<"Launch DaHua IPC"<<endl;
			rtsp = string("rtsp://") + user + ":" + passwd + "@" + ip + ":554/cam/realmonitor?channel=1&subtype=0";
		}else{
			cout<<"Launch HaiKang IPC"<<endl;
			rtsp= string("rtsp://") + user + ":" + passwd + "@" + ip + ":554/ch0/main/av_stream";
		}
		ts.push_back(thread(
			[=](){
				VideoCapture sequence(rtsp);
				if (!sequence.isOpened()){
					cerr << "Failed to open the image sequence!" << endl;
					return 1;
				}
                std::unique_ptr<tensorflow::Session> session;
                cout << "********** start a tensorflow session **********************" << endl;
                
                
                CascadeClassifier face_cascade;
                String face_cascade_name = "haarcascade_frontalface_alt.xml";
                if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading face cascade\n"); return -1; };
	
                
                Mat image;
                if (is_gui_present)
                    namedWindow(ip, 1);

                std::atomic<bool> consumed;
                consumed = true;
                thread processd;
                std::atomic<int> fps_process_cnt;
                std::atomic<int> fps_capture_cnt;
                fps_process_cnt = fps_capture_cnt = 0;

                chrono::steady_clock::time_point steady_start,steady_end;
                steady_start = chrono::steady_clock::now();
                steady_end = chrono::steady_clock::now();

                const int fps_duration = 2;
                Mat cur_image;
				for(;;){
                    cout<<"---------------------"<<endl;
					sequence >> image;

					if(image.empty()){
						cout << "End of Sequence" << endl;
						break;
					}
                    if (consumed){
                        if (processd.joinable())
                            processd.join();
                        cur_image = image.clone();
                        consumed = false;
                        processd = thread([&,mac](){
                                cout<<"##launching process thread"<<endl;
                                // detectAndGuess(cur_image);
                                LOG(INFO) << "========================= currently, dataset size = " << global_id_dataset.size() << endl;
                                vector<Meta_t>&& metas = detectAndGuess(cur_image, &session, face_cascade);
#ifdef ENABLE_JSON_POST
                             //    for (Meta_t& meta:metas){
                             //        meta.storeID="00001";
                             //        meta.mac=mac;
                             //        meta.uniqueID=meta.storeID;

                             //        string json = convertMeta2Json_rj(meta);
                             //        string output;
                             //        int ret = postJson2Server(json, output);
                             //        if (ret){
                             //            cout<<"Fail to do postJson2Server"<<endl;
                             //            exit(1);
                             //        }
                             //        else
                             //            // cout<<"postJson2Server return "<<output<<endl;
                             //            cout << "post json to server" << endl;
                             //    }
#endif
                                consumed=true;
                                fps_process_cnt++;


                        });
                    }
                    fps_capture_cnt++;

                    if (is_gui_present)
					{
						lock_guard<mutex> lock(m);
						// imshow(ip, image);
                        imshow(ip, cur_image);
						waitKey(1);
					}


                    steady_end = chrono::steady_clock::now();
                    if (chrono::duration<double>(steady_end - steady_start).count() >= fps_duration) {
                        long current_process = fps_process_cnt*1.0/fps_duration;
                        long current_capture = fps_capture_cnt*1.0/fps_duration;
                        fps_process_cnt = 0;
                        fps_capture_cnt = 0;

                        cout << "*** ---------------------------------------------------------------------------FPS thread-"
                                << this_thread::get_id()
                                << ": (capture_fps " << current_capture << ", " << "process_fps " << current_process << endl;
                        steady_start = steady_end;
                    }

				}
			}
		));
	}

	for (auto& t:ts){
		t.join();
	}

	return 0;
}


vector<Meta_t> detectAndGuess(Mat frame, std::unique_ptr<tensorflow::Session> *session, CascadeClassifier face_cascade)
{
    vector<Meta_t> facesMeta;
 
   
    std::vector<Rect> faces;
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    double scale = 0.50;
    Size dsize = Size(frame_gray.cols*scale, frame_gray.rows*scale);  
    Mat small_gray = Mat(dsize, CV_32S);
    resize(frame_gray, small_gray, dsize);
    equalizeHist(small_gray, small_gray);
    face_cascade.detectMultiScale(small_gray, faces, 1.1, 4,  0|CASCADE_SCALE_IMAGE, Size(15,15) );
    printf("have face %d \n", faces.size());
    // add recognition and guess here, we can ignore multiple faces at this time, so comment this for block.
    
    for ( size_t i = 0; i < faces.size(); i++ )
    {
        Meta_t oneMeta;
//         rectangle(frame,cvPoint(faces[i].x*int(1/scale),faces[i].y*int(1/scale)), cvPoint(faces[i].x*int(1/scale) + faces[i].width*int(1/scale), faces[i].y*int(1/scale) + faces[i].height*int(1/scale)),Scalar(255,0,255),1,1,0);
        
        int ROIheight = small_gray.rows;
        int ROIwidth = faces[i].width;
        int ROIx = faces[i].x;
        int ROIy = faces[i].y - faces[i].height * 0.4;
        if(ROIy <= 0) ROIy = 0;

        if( faces[i].y+faces[i].height * 1.4 >= small_gray.rows)
        {
            ROIheight = small_gray.rows - faces[i].y - faces[i].height * 0.2;
        }
        else
        {
            ROIheight = faces[i].height * 1.4;
        }
        if( ROIwidth * ROIheight < small_gray.cols * small_gray.rows / 8 )
        {
            // if face size < 1/8 frame size, ignore
            LOG(INFO) << " big size = " << small_gray.cols << " x " << small_gray.rows << endl; 
            LOG(INFO) << "small size = " << ROIwidth * ROIheight << " big size = " <<  small_gray.cols * small_gray.rows << endl;
            // continue;
        }
        cout << "=-=-= original x=" << faces[i].x << " y=" << faces[i].y << " width=" << faces[i].width << " height=" << faces[i].height << endl;
        faces[i].x = ROIx * int(1/scale);
        faces[i].y = ROIy * int(1/scale);
        faces[i].width =  ROIwidth * int(1/scale);
        faces[i].height = ROIheight * int(1/scale);
        LOG(INFO) << "=-=-= new x=" << faces[i].x << " y=" << faces[i].y << " width=" << faces[i].width << " height=" << faces[i].height << endl;
        cv::Mat image_mat = frame(faces[i]);
        rectangle(frame, cvPoint(faces[i].x, faces[i].y), cvPoint(faces[i].x+faces[i].width, faces[i].y+faces[i].height), Scalar(225,0,200),1,1,0);
        Eigen::ArrayXf embs_1 = GetEmbeddingsUsingCvMat(image_mat, "face_recog/model/20170512-110547.pb");
        int is_known = IsKnown(embs_1, global_id_dataset);
        LOG(INFO) << "========================= currently, dataset size = " << global_id_dataset.size() << "================is_known is" << is_known << endl;
        if (is_known != -1)
        {
            LOG(INFO) << "Customer: " << id_name[is_known];
            //oneMeta.customerID = 0;维护一个id的数组
        }
        else
        {
            string name = "id_" + std::to_string(id_name.size());
            // Don't need to add to global here, because it will be added after sending data to cloud
            global_id_dataset.push_back(embs_1);
            id_name.push_back(name);
            LOG(INFO) << "Unknown customer. Add to dataset, name: " << name;
            oneMeta.customerID = -1;
            oneMeta.age = 1;//ClassifyAgeUsingCvMat(image_mat, session);
            oneMeta.gender = 1;//ClassifyGenderUsingCvMat(image_mat, session);
            for(int j=0;j<128;j++)
            {
                oneMeta.feature[j]=embs_1.data()[j];
            }
        }
        oneMeta.type = 0;
        oneMeta.roi[0] = faces[i].x;
        oneMeta.roi[1] = faces[i].y;
        oneMeta.roi[2] = faces[i].width;
        oneMeta.roi[3] = faces[i].height;
        auto currently = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
        cout << currently << " time" << endl;
        oneMeta.timeStamp = currently;
        oneMeta.customerID = 0;
        facesMeta.push_back(oneMeta);
    }
    cout << duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count() << "   time " << endl;
    return facesMeta;
}
