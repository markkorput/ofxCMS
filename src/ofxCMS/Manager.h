#pragma once

#ifdef OFXCMS_JSON
    #include "JsonParser.h"
#endif

namespace ofxCMS {
    template<class CollectionClass>
    class Manager {
        private:
            //! a shared_ptr for a global singleton instance
            static shared_ptr<Manager<CollectionClass>> _singleton_ref;

        public:

            //! method to get the global singleton instance
            inline static shared_ptr<Manager<CollectionClass>> singletonRef();
            //! method to explicitly destroy the singleton instance
            inline static void deleteSingletonRef();

        public:

            void loadJsonFromFile(const string& path);
            shared_ptr<CollectionClass> get(const string& name);
    };
}

// IMPLEMENTATION OF ofxCMS::Manager TEMPLATE CLASS

template<class CollectionClass>
shared_ptr<Manager<CollectionClass>> ofxCMS::Manager<CollectionClass>::singletonRef(){
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
void ofxCMS::Manager<CollectionClass>::loadJsonFromFile(const string& path){

}

template<class CollectionClass>
shared_ptr<CollectionClass> ofxCMS::Manager<CollectionClass>::get(const string& name){
    return nullptr;
}
