#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "meta.h"

using namespace std;


#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
using namespace rapidjson;

string convertMeta2Json_rj(const Meta_t& meta){
    // document is the root of a json message
    rapidjson::Document doc;

    // define the document as an object rather than an array
    doc.SetObject();

    // create a rapidjson array type with similar syntax to std::vector
    rapidjson::Value array_roi(rapidjson::kArrayType);
    rapidjson::Value array_feature(rapidjson::kArrayType);

    // must pass an allocator when the object may need to allocate memory
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember(Value().SetString("storeID", allocator),     Value().SetString(meta.storeID.c_str(), allocator),      allocator);
    doc.AddMember(Value().SetString("mac", allocator),         Value().SetString(meta.mac.c_str(), allocator),          allocator);
    doc.AddMember(Value().SetString("uniqueID", allocator),    Value().SetString(meta.uniqueID.c_str(), allocator),     allocator);
    doc.AddMember(Value().SetString("type", allocator),        Value().SetInt(meta.type),                               allocator);
    doc.AddMember(Value().SetString("gender", allocator),      Value().SetInt(meta.gender),                             allocator);
    doc.AddMember(Value().SetString("age", allocator),         Value().SetInt(meta.age),                                allocator);
    doc.AddMember(Value().SetString("timeStamp", allocator),   Value().SetUint64(meta.timeStamp),                       allocator);
    doc.AddMember(Value().SetString("customerID", allocator),  Value().SetInt(meta.customerID),                         allocator);

    for (int i=0;i<4;i++){
        array_roi.PushBack(rapidjson::Value().SetInt(meta.roi[i]), allocator);
    }
    doc.AddMember("roi", array_roi, allocator);

    for (int i=0;i<128;i++){
        array_feature.PushBack(rapidjson::Value().SetDouble(meta.feature[i]), allocator);
    }
    doc.AddMember("feature", array_feature, allocator);

    rapidjson::StringBuffer strbuf;
    strbuf.Clear();

    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    doc.Accept(writer);

    cout<<__FUNCTION__<<" ---->" <<endl;
    cout<<strbuf.GetString()<<endl;

    return strbuf.GetString();
}


#ifdef BOOST_PROP

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

void example() {
    // Write json.
    ptree pt;
    pt.put ("foo", "bar");
    std::ostringstream buf;
    write_json (buf, pt, false);
    std::string json = buf.str(); // {"foo":"bar"}

    // Read json.
    ptree pt2;
    std::istringstream is (json);
    read_json (is, pt2);
    std::string foo = pt2.get<std::string> ("foo");
}

std::string map2json (const std::map<std::string, std::string>& map) {
    ptree pt;
    for (auto& entry: map)
        pt.put (entry.first, entry.second);
    std::ostringstream buf;
    write_json (buf, pt, false);
    return buf.str();
}

string convertMeta2Json_boost(const Meta_t& meta){
    ptree root;
    root.put("storeID",     meta.storeID);
    root.put("mac",         meta.mac);
    root.put("uniqueID",    meta.uniqueID);
    root.put("type",        meta.type);
    root.put("gender",      meta.gender);
    root.put("age",         meta.age);
    root.put("timeStamp",   meta.timeStamp);
    root.put("customerID",  meta.customerID);
    ptree rois;
    for (int i=0;i<4;i++){
        ptree node;
        node.put("",meta.roi[i]);
        rois.push_back(std::make_pair("", node));
    }
    root.add_child("roi",         rois);

    ptree features;
    for (int i=0;i<128;i++){
        ptree node;
        node.put("",meta.feature[i]);
        features.push_back(std::make_pair("", node));
    }
    root.add_child("feature",    features);

    std::ostringstream buf;
    write_json(buf, root, false);
    string json = buf.str();

    //print to screen
    cout<<__FUNCTION__<<" ---->" <<endl;
    write_json(std::cout, root);

    return json;
}
#endif

std::string JsonToString(rapidjson::Document &jsonObject) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(buffer);
    jsonObject.Accept(jsonWriter);
    return buffer.GetString();
}


void JsonToFile(rapidjson::Document &jsonObject, std::string fullpath) {
    std::ofstream outputFile;
    outputFile.open(fullpath);
    if(outputFile.is_open()) {
        std::string jsonObjectData = JsonToString(jsonObject);
        outputFile << jsonObjectData;
    }
    outputFile.close();
}


#ifdef DEBUG
int main(){

    string result;
    Meta_t meta;
#ifdef BOOST_PROP
    result = convertMeta2Json_boost(meta);
    cout<<result<<endl;
#endif

    result = convertMeta2Json_rj(meta);


    rapidjson::Document document;
    if (document.Parse<0>(result.c_str()).HasParseError()){
        cout << "json string parse error" <<endl;
        return 1;
    }
    JsonToFile(document, string("./output_jsonfile.json"));

    if (document.HasMember(Str_customerID)){
		Value &val = document[Str_customerID];
		//assert(val.IsString());
		//IPDomain = val.GetString();
		assert(val.IsInt());
		int customerId = val.GetInt();
		cout<<"customerId from json: :" << customerId <<endl;
    }

    return 0;
}
#endif
