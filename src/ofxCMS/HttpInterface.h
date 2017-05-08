#pragma once

#include "ManagerBase.h"

namespace ofxCMS {
    template<class CollectionClass>
    class HttpInterface {
    public:
        HttpInterface() : manager(NULL){}
        void setup(const string& host, int port, ManagerBase<CollectionClass>* manager);

    private:
        ManagerBase<CollectionClass>* manager;
        string host;
        int port;
    };
}

template<class CollectionClass>
void ofxCMS::HttpInterface<CollectionClass>::setup(const string& host, int port, ManagerBase<CollectionClass>* manager){
    this->host = host;
    this->port = port;
    this->manager = manager;
}
