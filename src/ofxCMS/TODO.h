//
//  CMSCollection.h
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#pragma once

#include "ofMain.h"
#include "ofxJSONElement.h"
#include "LambdaEvent.h"
// Even though ofxCMS::Collection is a template class,
// it does assume that any used model-type inherits from CMS::Model
#include "Model.h"

namespace ofxCMS {


    // Collection class that manages a collections of Models,
    // kinda based on the Backbone.js Collection
    template<class ModelClass>
    class Collection{

    public: // methods

        const static int INVALID_INDEX = -1;

        Collection() : _syncSource(NULL), mLimit(NO_LIMIT), bFIFO(false), mNextId(1){}
        ~Collection();

        void clone(Collection<ModelClass> &source);
        void syncsFrom(Collection<ModelClass> &collection, bool clearFirst = true);
        void stopSyncing();

        void limit(int amount){
            // apply limit to current collection
            while(count() > amount){
                remove(count()-1));
            }

            // save limit to be enforced in future additions
            mLimit = amount;
        }

        bool limitReached(){
            return mLimit != NO_LIMIT && count() >= mLimit;
        }

        void setFifo(bool fifo){ bFIFO = fifo; }
        bool getFifo(){ return bFIFO; }

        void setDestroyOnRemove(bool enable = true){ bDestroyOnRemove = enable; }
        bool getDestroyOnRemove(){ return bDestroyOnRemove; }

        bool has(shared_ptr<ModelClass> m){ return index(m) != INVALID_INDEX; }
        int index(shared_ptr<ModelClass> model){
            int i=0;
            for(auto ref : modelRefs){
                if(ref->equals(model))
                    return i;
                i++;
            }
            return INVALID_INDEX;
        }

        void shuffle(){
            int c = count();
            for(int i=c-1; i>=0; i--){
                auto tmp = modelRefs.back();
                modelRefs.pop_back(); // remove the last one
                modelRefs.insert(modelRefs.begin() + ofRandom(c-1), tmp); // insert at random position
            }
        }

        shared_ptr<ModelClass> getRef(ModelClass &instance);


    public: // filter methods

        // One-time filter: only keep models that have a specific key-value combination
        void filterBy(const string &key, const string &val);

        // One-time filter: only keep models with any of the specified values for a specific key
        void filterBy(const string &key, vector<string> &values);

        // Active Filter: only keep models with a specific key-value combination
        // and also apply this filter when new models are added
        void filtersBy(const string &attr, const string &value){
            // apply filter on current collection
            filterBy(attr, value);
            // save filter to apply to newly added models
            filterValues[attr] = value;
        }

        // Active Filter: only keep models with any of the specified values for a specific key
        // and also apply this filter when new models are added
        void filtersBy(const string &attr, vector<string> &values){
            // apply filter on current collection
            filterBy(attr, values);
            // save filter to apply to newly added models
            filterVectors[attr] = values;
        }

        // One-time filter: rejection only keep models that DO NOT have a specific key-value combination
        void rejectBy(const string &key, const string &val){
            // we have to do this backwards! because every time you remove a model,
            // it messes with all the following index values
            for(int i=count()-1; i>=0; i--){
                // get current model
                auto model = at(i);
                // remove it, if it doesn't meet the criteria
                if(!modelPassesSingleValueRejection(model, key, val)) remove(model);
            }
        }

        // one-time multi-value rejection; all models who's attribute match any of the value are removed
        void rejectBy(const string &key, vector<string> &values){
            // we have to do this backwards! because every time you remove a model,
            // it messes with all the following index values
            for(int i=count()-1; i>=0; i--){
                // get current model
                auto model = at(i);
                // remove it, if it doesn't meet the criteria
                if(!modelPassesMultiValueRejection(model, key, values)) remove(model);
            }
        }

        // Active Filter: only keep models without a specific key-value combination
        // and also apply this filter when new models are added
        void rejectsBy(const string &attr, const string &value){
            // apply filter on current collection
            rejectBy(attr, value);
            // save filter to apply to newly added models
            rejectValues[attr] = value;
        }

        // Active Filter: only keep models without any of the specified values for a specific key
        // and also apply this filter when new models are added
        void rejectsBy(const string &attr, vector<string> &values){
            // apply filter on current collection
            rejectBy(attr, values);
            // save filter to apply to newly added models
            rejectVectors[attr] = values;
        }

        void removeFilters(bool resync = true){
            filterValues.clear();
            filterVectors.clear();
            rejectValues.clear();
            rejectVectors.clear();

            // if we're syncing from a collection, the a clean sync, without the just removed filters
            if(resync && _syncSource){
                clear();
                clone(*_syncSource);
            }
        }

        void removeFilter(const string &attr, bool resync = true){
            filterValues.erase(attr);
            filterVectors.erase(attr);
            rejectValues.erase(attr);
            rejectVectors.erase(attr);

            // if we're syncing from a collection, the a clean sync, without the just removed filters
            if(resync && _syncSource){
                clear();
                clone(*_syncSource);
            }
        }

    protected: // filter methods

        bool modelPassesActiveFilters(shared_ptr<ModelClass> model){
            // single value filters
            for (map<string, string>::iterator it = filterValues.begin(); it!=filterValues.end(); it++){
                if(!modelPassesSingleValueFilter(model, it->first, it->second)){
                    return false;
                }
            }

            // multi value filters
            for (map< string, vector<string> >::iterator it = filterVectors.begin(); it!=filterVectors.end(); it++){
                if(!modelPassesMultiValueFilter(model, it->first, it->second)){
                    return false;
                }
            }

            return true;
        }

        bool modelPassesMultiValueFilter(shared_ptr<ModelClass> model, const string &attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return true;
            }

            return false;
        }

        bool modelPassesSingleValueFilter(shared_ptr<ModelClass> model, const string &attr, const string &value){
            return model->get(attr) == value;
        }

        bool modelPassesActiveRejections(shared_ptr<ModelClass> model){
            // single value filters
            for (map<string, string>::iterator it = rejectValues.begin(); it!=rejectValues.end(); it++){
                if(!modelPassesSingleValueRejection(model, it->first, it->second)){
                    return false;
                }
            }

            // multi value filters
            for (map< string, vector<string> >::iterator it = rejectVectors.begin(); it!=rejectVectors.end(); it++){
                if(!modelPassesMultiValueRejection(model, it->first, it->second)){
                    return false;
                }
            }

            return true;
        }

        bool modelPassesMultiValueRejection(shared_ptr<ModelClass> model, const string &attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return false;
            }

            return true;
        }

        bool modelPassesSingleValueRejection(shared_ptr<ModelClass> model, const string &attr, string value){
            return model->get(attr) != value;
        }

    protected: // callbacks

        // NOTE: Model& type, not ModelClass& (see comments at implementation)
        void onSyncSourceModelAdded(ModelClass &m);
        void onSyncSourceModelChanged(AttrChangeArgs &args);
        void onSyncSourceModelRemoved(ModelClass &m);
        void onSyncSourceDestroying(Collection<ModelClass> &syncSourceCollection);

    public: // events

        ofEvent <void> collectionInitializedEvent;
        ofEvent < Collection<ModelClass> > collectionDestroyingEvent;

        ofEvent <ModelClass> modelRejectedEvent;
        ofEvent < Collection<ModelClass> > fifoEvent;

    protected: // attributes

        vector<shared_ptr<ModelClass>> modelRefs;
        Collection<ModelClass>* _syncSource;
        map<string, string> filterValues;
        map< string, vector<string> > filterVectors;
        map<string, string> rejectValues;
        map< string, vector<string> > rejectVectors;

        int mLimit;
        // first in first out; if true: when limit is reached, first element gets removed
        // instead of new elements being rejected
        bool bFIFO;
        // destroy models when removing them fmor the collection? (default: false)
        bool bDestroyOnRemove;
        int mNextId;

    }; // class Collection
} // namespace ofxCMS

// TEMPLATE CLASS IMPLEMENTATION (there's no .cpp files) //


template <class ModelClass>
ofxCMS::Collection<ModelClass>::~Collection(){
    ofNotifyEvent(collectionDestroyingEvent, *this, this);
    // do this first!
    stopSyncing();
    clear();
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::Collection<ModelClass>::findByAttr(const string &attr, const string &value){
    for(auto modelRef : modelRefs)
        if(modelRef->get(attr) == value)
            return modelRef;

    return nullptr;
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::Collection<ModelClass>::findById(const string &_id){
    for(auto modelRef : modelRefs)
        if(modelRef->id() == _id)
            return modelRef;

    return nullptr;
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::filterBy(const string &key, const string &val){
    // we have to do this backwards! because every time you remove a model,
    // it messes with all the following index values
    for(int i=modelRefs.size()-1; i>=0; i--){
        // get current model
        auto modelRef = at(i);
        // remove it, if it doesn't meet the criteria
        if(!modelPassesSingleValueFilter(modelRef, key, val)) remove(modelRef);
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::filterBy(const string &key, vector<string> &values){
    // we have to do this backwards! because every time you remove a model,
    // it messes with all the following index values
    for(int i=modelRefs.size()-1; i>=0; i--){
        // get current model
        auto modelRef = at(i);
        // remove it, if it doesn't meet the criteria
        if(!modelPassesMultiValueFilter(modelRef, key, values)) remove(modelRef);
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::clone(Collection<ModelClass> &source){
    clear(); // triggers modelRemovedEvents for each model
    for(int i=0; i<source.models().size(); i++){
        add(source.at(i)); // trigges modelAddedEvents
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::syncsFrom(Collection<ModelClass> &collection, bool clearFirst){
    // first, UNregister existing sync source callbacks
    stopSyncing();
    // we'll need this at destructor-time to unregister event callbacks
    _syncSource = &collection;
    // clear collection before syncing with new sync source
    if(clearFirst) clear();
    // first, clone the current content
    clone(*_syncSource);
    // second, register callbacks to stay up-to-date on later changes
    registerSyncCallbacks(*_syncSource);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::stopSyncing(){
    if(_syncSource){
        registerSyncCallbacks(*_syncSource, false);
        _syncSource = NULL;
    }
}


template <class ModelClass>
void ofxCMS::Collection<ModelClass>::onSyncSourceModelRemoved(ModelClass &model){
    // we could just do `remove(&model);` here, but even though that model checks if the model exists in this collection,
    // it kinda assumes it does and logs a warning message when this is not the case. Since it's very likely that a model
    // that we receive in this callback function is NOT part of our collection, we perform this check here.
    int idx = index(&model);
    if(idx != INVALID_INDEX)
        remove(idx);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::onSyncSourceModelAdded(ModelClass &m){
    // if our sync source gets a new model, we follow...
    // note that our add() function does apply all the active filters/rejections,
    // so the model might not actually end up in our collection
    add(&m);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::onSyncSourceModelChanged(AttrChangeArgs &args){
    if(args.model == NULL){
        ofLogWarning() << "onSyncSourceModelChanged - got NULL model";
        return;
    }

    // get internal reference to model
    auto ref = getRef(*args.model);
    if(ref == nullptr){
        ofLogWarning() << "got model change from sync source for unknown model";
        return;
    }

    //
    // Use active filters and rejections to re-evaluate if the syncSource's model
    // should or should not be in our collection after the change to its properties
    //

    // see if the model passes our active filter and rejection rules
    bool pass = modelPassesActiveFilters(ref) && modelPassesActiveRejections(ref));

    if(this->has(ref)){
        // already in our collection;
        // if after the change the model does NOT pass our filters; remove it
        if(!pass)
            remove(ref);

        return;
    }

    // not yet in our collection?
    // if after the change the model DOES pass our filters; add it
    if(pass)
        add(ref);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::onSyncSourceDestroying(Collection<ModelClass> &syncSourceCollection){
    stopSyncing();
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::Collection<ModelClass>::getRef(ModelClass &instance){
    for(auto modelRef : modelRefs)
        if(instance.equals(modelRef))
            return modelRef;
    return nullptr;
}
