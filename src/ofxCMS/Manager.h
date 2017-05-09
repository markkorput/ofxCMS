#pragma once

#include "ManagerBase.h"
#include "HttpInterface.h"

#ifdef OFXCMS_JSON
    #include "JsonParserManager.h"
#endif



#define CMSMAN_INIT template<>\
shared_ptr<ofxCMS::Manager<ofxCMS::Collection<ofxCMS::Model>>>\
    ofxCMS::Manager<ofxCMS::Collection<ofxCMS::Model>>::_singleton_ref = nullptr;

#define CMSMAN ofxCMS::Manager<ofxCMS::Collection<ofxCMS::Model>>::singletonRef()
#define CMSMAN_DELETE ofxCMS::Manager<ofxCMS::Collection<ofxCMS::Model>>::deleteSingletonRef()

namespace ofxCMS {
    template<class CollectionClass>
    class Manager : public ManagerBase<CollectionClass>{

        private:
            //! a shared_ptr for a global singleton instance
            static shared_ptr<Manager<CollectionClass>> _singleton_ref;

        public:

            //! method to get the global singleton instance
            inline static shared_ptr<Manager<CollectionClass>> singletonRef();
            //! method to explicitly destroy the singleton instance
            inline static void deleteSingletonRef();

            shared_ptr<HttpInterface<CollectionClass>> getHttpInterface(const string& host, int port);

#ifdef OFXCMS_JSON
            bool loadJsonFromFile(const string& path);
#endif


    };
}

// IMPLEMENTATION OF ofxCMS::Manager TEMPLATE CLASS

template<class CollectionClass>
shared_ptr<ofxCMS::Manager<CollectionClass>> ofxCMS::Manager<CollectionClass>::singletonRef(){
    if(!_singleton_ref){
        // ofLogVerbose() << "Creating singleton of class ofxCMS::Manager";
        _singleton_ref = make_shared<Manager<CollectionClass>>();
    }
    return _singleton_ref;
}

template<class CollectionClass>
void ofxCMS::Manager<CollectionClass>::deleteSingletonRef(){
    if(_singleton_ref){
        _singleton_ref.reset();
        _singleton_ref = nullptr;
    }
}

template<class CollectionClass>
shared_ptr<ofxCMS::HttpInterface<CollectionClass>> ofxCMS::Manager<CollectionClass>::getHttpInterface(const string& host, int port){
    auto httpRef = make_shared<HttpInterface<CollectionClass>>();
    httpRef->setup(host, port, this);
    return httpRef;
}

#ifdef OFXCMS_JSON

template<class CollectionClass>
bool ofxCMS::Manager<CollectionClass>::loadJsonFromFile(const string& path){
    JsonParserManager<CollectionClass> jsonParser;
    jsonParser.setup(this, path);
    return jsonParser.load();
}

#endif
