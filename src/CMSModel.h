//
//  CMSModel.h
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#ifndef __ofxCMS__CMSModel__
#define __ofxCMS__CMSModel__

#include "ofMain.h"

namespace CMS {

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

        Model* set(string attr, string value, bool notify = true);
        Model* set(map<string, string> &attrs);
        string get(string attr, string _default = "");
        string id();
        string cid();
        map<string, string> &attributes(){ return _attributes; }

        void destroy(bool notify = true);

    public: // static helpers

        static vector<string> jsonArrayToIdsVector(string jsonText);
        static vector<string> jsonArrayToStringVector(string jsonText);

    public: // events

        ofEvent <AttrChangeArgs> attributeChangedEvent;
        ofEvent <Model> beforeDestroyEvent;

    protected: // callbacks
        
        virtual void onSetAttribute(string attr, string value){}
        virtual void onAttributeChanged(string attr, string value, string old_value){}

    protected:

        map<string, string> _attributes;

        // CID stuff (client-id, local/internal ids,
        // mainly to identify unpersisted models)
        string mCid;
        static int mCidCounter; // to give every model its own

    }; // class Model
    
}; // namespace CMS

#endif /* defined(__ofxCMS__CMSModel__) */
