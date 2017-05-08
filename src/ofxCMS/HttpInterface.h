#pragma once

#include "lib/lambda.h"
#include "ManagerBase.h"

namespace ofxCMS {

    namespace Http {
        class Response {
        public:
            Response(){
                collection = make_shared<Collection<Model>>();
            }

            shared_ptr<Collection<Model>> collection;
        };
    }

    class QueryBuilder {
    public:
        typedef FUNCTION<void(shared_ptr<Http::Response>)> QueryResultFunc;
    public:
        void setCollection(const string& name){ collection = name; }
        QueryBuilder* get(){ action = "get"; return this; }
        QueryBuilder* onSuccess(QueryResultFunc func){ successFuncs.push_back(func); return this; }
        QueryBuilder* send(){
            ofLog() << "TODO: send query and put results in repsonse->collection";

            auto responseRef = make_shared<Http::Response>();

            for(auto func : successFuncs){
                func(responseRef);
            }
            return this;
        }

    private:
        string collection;
        string action;
        vector<QueryResultFunc> successFuncs;
    };

    template<class CollectionClass>
    class HttpInterface {
    public:
        HttpInterface() : manager(NULL){}
        void setup(const string& host, int port, ManagerBase<CollectionClass>* manager);

        shared_ptr<QueryBuilder> collection(const string& name);

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

template<class CollectionClass>
shared_ptr<QueryBuilder> ofxCMS::HttpInterface<CollectionClass>::collection(const string& name){
    auto queryBuilderRef = make_shared<QueryBuilder>();
    queryBuilderRef->setCollection(name);
    return queryBuilderRef;
}
