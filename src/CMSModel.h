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

    // used in attributeChangeEvent notifications
    class AttrChangeArgs {
    public:
        string attr;
        string value;
    };
    
    // a key-value pair model that fires notifications when attributes change,
    // kinda based on the Backbone.js Models
    class Model{

    public:
        Model();
        ~Model();

        Model* set(string attr, string value, bool notify = true);
        string get(string attr, string _default = "");
        string id();
        string cid();
        map<string, string> &attributes(){ return _attributes; }


        // void destroy(bool notify = true);

        ofEvent <AttrChangeArgs> attributeChangedEvent;
        ofEvent <Model> beforeDestroyEvent;

    protected:
        
        vector<string> jsonArrayToIdsVector(string jsonText);
        virtual void onSetAttribute(string attr, string value){}

    protected:

        map<string, string> _attributes;

        // CID stuff (client-id, local/internal ids,
        // mainly to identify unpersisted models)
        string mCid;
        static int mCidCounter; // to give every model its own

    }; // class Model
    
}; // namespace CMS

#endif /* defined(__ofxCMS__CMSModel__) */
