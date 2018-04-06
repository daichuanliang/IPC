#all:
#	g++ -std=c++11 -DUSE_BOOST ipc.cpp face_recog/age_gender_classify.cpp face_recog/face_recognition.cpp face_recog/tensor_util.cpp -o ipc -I/usr/local/include/tf -I/usr/local/include/tf/bazel-genfiles -I/root/Documents/tf_jiang/tensorflow-master/bazel-tensorflow-master/external/nsync/public -I/home/tony/work/eigen/eigen -L/usr/local/lib/libtensorflow_cc `pkg-config --cflags --libs opencv protobuf` -ltensorflow_cc -ltensorflow_framework -pthread -lboost_regex
#	#g++ -std=c++11 -DUSE_BOOST ipc.cpp -o ipc -lopencv_core -lopencv_imgproc -lopencv_highgui -pthread -lboost_regex



CXXFLAGS = -std=c++11 -Wall -Wno-strict-aliasing -Wno-unused-variable

#包含头文件路径
SUBDIR   = $(shell ls ./src -R | grep /)
SUBDIRS  = $(subst :,/,$(SUBDIR))
#TODO: move to dedicated folder
INCPATHS += -I./ -I./include -I/usr/local/include/tf -I/usr/local/include/tf/bazel-genfiles -I/root/Documents/tf_jiang/tensorflow-master/bazel-tensorflow-master/external/nsync/public -I/home/tony/work/eigen/eigen -L/usr/local/lib/libtensorflow_cc 
SOURCE = $(foreach dir,$(SUBDIRS),$(wildcard $(dir)*.cpp))
SOURCE += face_recog/age_gender_classify.cpp face_recog/face_recognition.cpp face_recog/tensor_util.cpp 


VPATH = $(subst : ,:,$(SUBDIR))./
OBJS = $(patsubst %.cpp,%.o,$(SOURCE))
OBJFILE  = $(foreach dir,$(OBJS),$(notdir $(dir)))
OBJSPATH = $(addprefix obj/,$(OBJFILE)) 


LDFLAGS_HICOM += -L./lib -lhcnetsdk -L./lib/HCNetSDKCom -lhpr -lHCCore -lX11
LDFLAGS_CV += `pkg-config --cflags --libs opencv protobuf`
LDFLAGS_CV += -pthread  -L/usr/lib/x86_64-linux-gnu -lboost_regex
LDFLAGS_CV += -ltensorflow_cc -ltensorflow_framework 
#for curl
LDFLAGS_JSON_CURL += -lcurl -DENABLE_JSON_POST


LDFLAGS += $(LDFLAGS_HICOM) $(LDFLAGS_CV) $(LDFLAGS_JSON_CURL)


EXE = ./ipc

$(EXE): $(SOURCE)
	g++ -std=c++11  -DUSE_BOOST -DRESET_CONFIG -o $(EXE) $(SOURCE)  $(INCPATHS) $(LDFLAGS) -DIPC
	#g++ -std=c++11  -DUSE_BOOST  -o $(EXE) $(SOURCE)  $(INCPATHS) $(LDFLAGS) -DIPC

.PHONY:clean all
clean:
	rm -rf $(OBJFILE)
	rm -rf $(EXE)

all: $(EXE)

debug: src/jsonAPI.cpp
	g++ -std=c++11  -DUSE_BOOST -DDEBUG -o jsonAPI  src/jsonAPI.cpp  $(INCPATHS) $(LDFLAGS) -DIPC
