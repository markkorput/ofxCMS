//
//  CMSModel.h
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#pragma once

#include "ofMain.h"
#include "LambdaEvent.h"

namespace ofxCMS {
    // a key-value pair model that fires notifications when attributes change,
    // kinda based on the Backbone.js Models
    class Model{

    public:

        // used in attributeChangeEvent notifications
        class AttrChangeArgs {
        public:
            Model *model;
            string attr;
            string value;
        };

    public:

        static const unsigned int INVALID_CID = 0;
        static unsigned int nextCid;
        Model();

        Model* set(const string &attr, const string &value, bool notify = true);
        Model* set(map<string, string> &attrs, bool notify=true);
        string get(const string &attr, string _default = "") const;

        string id() const { return get("id", get("_id", getId())); }
        string getId() const { return mId; }
        void setCid(unsigned int newCid){ mCid = newCid; }
        unsigned cid() const { return mCid; }
        unsigned int getCid() const { return mCid; }

        map<string, string> &attributes(){ return _attributes; }

        bool equals(shared_ptr<Model> other){ return other->cid() == cid(); }

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

        // id; for identifying models across different platforms
        string mId;

        // cid (client-id, for local/internal use)
        unsigned int mCid;

    }; // class Model

}; // namespace CMS
