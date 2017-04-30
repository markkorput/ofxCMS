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

    class Model;

    // used in attributeChangeEvent notifications
    class AttrChangeArgs {
    public:
        Model *model;
        string attr;
        string value;
    };

    // a key-value pair model that fires notifications when attributes change,
    // kinda based on the Backbone.js Models
    class Model{

    public:
        Model();
        // ~Model();

        void setId(const string& newId){ mCid = newId; }
        Model* set(const string &attr, const string &value, bool notify = true);
        Model* set(map<string, string> &attrs, bool notify=true);
        string get(const string &attr, string _default = "");
        string id();
        string cid();
        map<string, string> &attributes(){ return _attributes; }

        void destroy(bool notify = true);

    public: // static helpers

        static vector<string> jsonArrayToIdsVector(string jsonText);
        static vector<string> jsonArrayToStringVector(string jsonText);

    public: // events

        LambdaEvent<AttrChangeArgs> attributeChangedEvent;
        ofEvent <Model> beforeDestroyEvent;

    protected: // callbacks

        virtual void onSetAttribute(const string &attr, const string &value){}
        virtual void onAttributeChanged(const string &attr, const string &value, const string &old_value){}

    protected:

        map<string, string> _attributes;

        // CID stuff (client-id, local/internal ids,
        // mainly to identify unpersisted models)
        string mCid;

    }; // class Model

}; // namespace CMS
