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
    class Model{

    public: // sub-types

        typedef void* CidType;

        typedef FUNCTION<void(const string&, const string&)> AttrIterateFunc;
        typedef FUNCTION<void(void)> LockFunctor;

        // used in attributeChangeEvent notifications
        class AttrChangeArgs {
        public:
            Model *model;
            string attr;
            string value;
        };

        //! Sub-type used a record-format when queueing modifications while to model is locked
        class Mod {
            public:
                string attr, value;
                bool notify;
                Mod(const string& _attr, const string& _value, bool _notify=true) : attr(_attr), value(_value), notify(_notify){}
        };

    public:

        Model() : lockCount(0){}
        Model* set(const string &attr, const string &value, bool notify = true);
        Model* set(map<string, string> &attrs, bool notify=true);
        string get(const string &attr, string _default = "") const;

        string getId() const { return get("id", get("_id")); }
        CidType cid() const { return (CidType)this; }

        const map<string, string> &attributes() const { return _attributes; }

        bool has(const string& attr) const;
        bool equals(shared_ptr<Model> other){ return other->cid() == cid(); }
        size_t size() const { return _attributes.size(); }

        void each(AttrIterateFunc func);
        void copy(shared_ptr<Model> otherRef, bool also_ids=false);
        void copy(Model& other, bool also_ids=false);

    protected: // methods

        bool isLocked() const { return lockCount > 0; }
        void lock(LockFunctor func);

    public: // static helpers

#ifdef OFXCMS_JSON
        static vector<string> jsonArrayToIdsVector(string jsonText);
        static vector<string> jsonArrayToStringVector(string jsonText);
#endif

    protected: // callbacks

        //! this virtual method is called whenever an attribute is written (using this->set()) and can be overwritten be inheriting classes
        virtual void onSetAttribute(const string &attr, const string &value){}
        //! this virtual method is called whenever an attribute is changed (using this->set()) and can be overwritten be inheriting classes
        virtual void onAttributeChanged(const string &attr, const string &value, const string &old_value){}

    public: // events

        //! this event is triggered whenever the model changes (which means; when any attribute changes) and gives the caller a reference to this model
        LambdaEvent<Model> changeEvent;
        //! this event is triggered whenever the model changes (which means; when any attribute changes) and gives the caller an object with a pointer to the model and information about which attribute changed
        LambdaEvent<AttrChangeArgs> attributeChangeEvent;

    private: // attributes

        //! the internal storage map for this model's attributes
        map<string, string> _attributes;
        // a counter to track the number of active (recursive) locks
        int lockCount;
        std::vector<shared_ptr<Mod>> modQueueRefs;

    }; // class Model

}; // namespace CMS
