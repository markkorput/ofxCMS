//
//  CMSModel.h
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#pragma once

#include "ofMain.h"
#include "ofxLambdaEvent/LambdaEvent.h"

#define OFXCMS_INVALID_CID 0

namespace ofxCMS {
    // a key-value pair model that fires notifications when attributes change,
    // kinda based on the Backbone.js Models
    class Model;

    typedef Model* CidType;


    class Model{

    public:

        typedef FUNCTION<void(const string&, const string&)> AttrIterateFunc;

        // used in attributeChangeEvent notifications
        class AttrChangeArgs {
        public:
            Model *model;
            string attr;
            string value;
        };

    public:

        Model* set(const string &attr, const string &value, bool notify = true);
        Model* set(map<string, string> &attrs, bool notify=true);
        string get(const string &attr, string _default = "") const;

        string getId() const { return get("id", get("_id")); }
        CidType cid() const { return (CidType)this; }

        const map<string, string> &attributes() const { return _attributes; }

        bool has(const string& attr) const;
        bool equals(shared_ptr<Model> other){ return other->cid() == cid(); }

        void each(AttrIterateFunc func);
        void copy(shared_ptr<Model> otherRef, bool also_ids=false);
        void copy(Model& other, bool also_ids=false);

    public: // static helpers
#ifdef OFXCMS_JSON
        static vector<string> jsonArrayToIdsVector(string jsonText);
        static vector<string> jsonArrayToStringVector(string jsonText);
#endif
    public: // events

        LambdaEvent<Model> changeEvent;
        LambdaEvent<AttrChangeArgs> attributeChangeEvent;

    protected: // callbacks

        virtual void onSetAttribute(const string &attr, const string &value){}
        virtual void onAttributeChanged(const string &attr, const string &value, const string &old_value){}

    private:

        map<string, string> _attributes;

    }; // class Model

}; // namespace CMS
