#pragma once

#include "lib/lambda.h"
#include "ManagerBase.h"
#ifdef OFXCMS_OFXSIMPLEHTTP
    #include "ofxSimpleHttp.h"
#endif

namespace ofxCMS {

    namespace Http {
        class Response {
        public:
            Response(){
                collection = make_shared<Collection<Model>>();
            }

            shared_ptr<Collection<Model>> collection;
#ifdef OFXCMS_OFXSIMPLEHTTP
            ofxSimpleHttpResponse* rawResponse;
#endif
        };
    }

    class QueryBuilder {
    public:
        typedef FUNCTION<void(shared_ptr<Http::Response>)> QueryResultFunc;

        QueryBuilder() : collection(""), action("get"){}
        void setCollection(const string& name){ collection = name; }
        QueryBuilder* onSuccess(QueryResultFunc func){ successFuncs.push_back(func); return this; }
        QueryBuilder* send(){
            sendEvent.notifyListeners(*this);
            return this;
        }

        string getURL(){
            string url = collection + "/";
            // std::map<string, string> params;
            // if limit params['limit'] = ofToString(limit)
            // for(auto pair : params) ...
            return url;
        }

        LambdaEvent<QueryBuilder> sendEvent;

        string collection;
        string action;
        vector<QueryResultFunc> successFuncs;
    };

    template<class CollectionClass>
    class HttpInterface {
    public:
        HttpInterface() : manager(NULL), rootUrl("/"){}
        void setup(const string& host, int port, ManagerBase<CollectionClass>* manager);
        void update();

        // start building a get request for the specified resource
        shared_ptr<QueryBuilder> get(const string& name);

        void setRootUrl(const string& url){ rootUrl = url; }

    private:
        std::vector<shared_ptr<QueryBuilder>> queryBuilderRefs;
        ManagerBase<CollectionClass>* manager;
        string host;
        int port;
        string rootUrl;

#ifdef OFXCMS_OFXSIMPLEHTTP
        ofxSimpleHttp http;
#endif
    };
}

template<class CollectionClass>
void ofxCMS::HttpInterface<CollectionClass>::setup(const string& host, int port, ManagerBase<CollectionClass>* manager){
    this->host = host;
    this->port = port;
    this->manager = manager;
}

template<class CollectionClass>
void ofxCMS::HttpInterface<CollectionClass>::update(){
#ifdef OFXCMS_OFXSIMPLEHTTP
    http.update();
#endif
}

template<class CollectionClass>
shared_ptr<ofxCMS::QueryBuilder> ofxCMS::HttpInterface<CollectionClass>::get(const string& name){
    // create query builder
    auto queryBuilderRef = make_shared<QueryBuilder>();
    queryBuilderRef->setCollection(name);


    queryBuilderRef->sendEvent.addListener([this](QueryBuilder& builder){
        string url = "http://"+this->host+":"+ofToString(this->port)+this->rootUrl+builder.getURL();

#ifndef OFXCMS_OFXSIMPLEHTTP
        ofLogWarning() << "OFXCMS_OFXSIMPLEHTTP not enabled, can't perform actual http request: " << url;
#else
        ofLogVerbose() << "HTTP GET: " << url;
        auto notifierRef = this->http.fetchURL(url, true /* yes, we do want notification on success */);

        notifierRef->onSuccess([this, &builder](ofxSimpleHttpResponse& httpResponse){
            // create our own response object
            auto responseRef = make_shared<Http::Response>();
            responseRef->rawResponse = &httpResponse;

            if(httpResponse.ok){
                // process http response json
                responseRef->collection->loadJson(httpResponse.responseBody);

                for(auto func : builder.successFuncs){
                    func(responseRef);
                }
            } else {
                // not ok
                ofLogWarning() << "Http response status: " << httpResponse.status << ", body: " << httpResponse.responseBody;
                ofLogWarning() << "TODO: error response callbacks";
            }

            ofLogWarning() << "TODO: general response callbacks";
            ofLogWarning() << "TODO: remove query builder from our internal vector";
            return this;
        });
#endif
    }, this);

    ofLogWarning() << "TODO: perform maintenance and remove all idle (orphaned) request builders";

    // keep track of active query builders
    queryBuilderRefs.push_back(queryBuilderRef);
    return queryBuilderRef;
}
