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

    // The CMS::Model / ofxCMS::Collection system is based on pointers,
    // and data is parsed straight into existing models, so there should
    // never be a situation when two different instances of the same model
    // appear in memory, so in most cases you'll be find with using SAME_MODEL
    // but to be thorough MODELS_MATCH could be used to check if the ids match instead (less strict)
    #define SAME_MODEL(a,b) (a == b)
    #define MODELS_MATCH(a,b) (a->id() == b->id())

    // Collection class that manages a collections of Models,
    // kinda based on the Backbone.js Collection
    template<class ModelClass>
    class Collection{

    public: // methods

        const static int NO_LIMIT = -1;
        const static int INVALID_INDEX = -1;

        Collection() : _syncSource(NULL), mLimit(NO_LIMIT), bFIFO(false), bDestroyOnRemove(false), mNextId(1){}
        ~Collection();

        void initialize(vector< map<string, string> > &_data);

        shared_ptr<ModelClass> create();

        bool add(ModelClass *model, bool notify = true);
        ModelClass* remove(ModelClass *model, bool doDestroy = true);
        ModelClass* remove(int index, bool doDestroy = true);
        void destroy(int index);
        void destroy(ModelClass *model);
        void destroyBy(const string &key, const string &value);
        void clear();
        void destroyAll();

        const vector<ModelClass*> &models();
        unsigned int count(){ return _models.size(); }

        ModelClass* at(unsigned int idx);
        ModelClass* findByAttr(const string &attr, const string &value);
        ModelClass* findById(const string &_id);
        ModelClass* byCid(const string &cid);
        int randomIndex(){ return _models.size() == 0 ? -1 : floor(ofRandom(_models.size())); }
        ModelClass* random(){ return _models.size() == 0 ? NULL : at(randomIndex()); }

        ModelClass* previous(ModelClass* model);
        ModelClass* next(ModelClass* model);

        void clone(Collection<ModelClass> &source);
        void syncsFrom(Collection<ModelClass> &collection, bool clearFirst = true);
        void stopSyncing();

        void limit(int amount){
            // apply limit to current collection
            while(_models.size() > amount){
                remove(at(_models.size()-1));
            }
            // save limit to be enforced in future additions
            mLimit = amount;
        }

        bool limitReached(){
            return mLimit != NO_LIMIT && _models.size() >= mLimit;
        }

        void setFifo(bool fifo){ bFIFO = fifo; }
        bool getFifo(){ return bFIFO; }

        void setDestroyOnRemove(bool enable = true){ bDestroyOnRemove = enable; }
        bool getDestroyOnRemove(){ return bDestroyOnRemove; }

        bool has(ModelClass* m){ return index(m) != INVALID_INDEX; }
        int index(ModelClass* m){
            for(int i=0; i<_models.size(); i++){
                if(MODELS_MATCH(_models[i], m))
                    return i;
            }

            return INVALID_INDEX;
        }

        void shuffle(){
            for(int i=count()-1; i>=0; i--){
                ModelClass* tmp = _models.back();
                _models.pop_back(); // remove the last one
                _models.insert(_models.begin() + ofRandom(_models.size()), tmp); // insert at random position
            }
        }
    public: // parsing methods

        bool parse(const string &jsonText, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
        bool parse(const ofxJSONElement & node, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
        void parseModelJson(ModelClass *model, const string &jsonText);

        // "merge" all models of another collection into our own collection.
        // for each model in the other collection, it will try to find an existing
        // model in our own collection (matching on model->id()). If found, that existing model
        // is updated with the attributes of the other collection's model. If NOT found,
        // a new model is created with the attributes of the other collection's model and
        // added to our collection
        void merge(Collection<ModelClass> &otherCollection){
            // loop over other collection's models
            for(int i=0; i<otherCollection.count(); i++){
                ModelClass* otherModel = otherCollection.at(i);
                if(otherModel == NULL) continue;

                // find existing matching model in our own collection
                ModelClass* existing = this->findById(otherModel->id());
                if(existing){
                    // update existing model
                    existing->set(otherModel->attributes());
                    // done
                    continue;
                }

                // no existing model found, create new model
                ModelClass* newModel = new ModelClass();
                // initialize new model with data from other model
                newModel->set(otherModel->attributes());
                // add it to our collection
                if(!add(newModel)){
                    delete newModel;
                }
            }
        }

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
            for(int i=_models.size()-1; i>=0; i--){
                // get current model
                ModelClass* model = at(i);
                // remove it, if it doesn't meet the criteria
                if(!modelPassesSingleValueRejection(model, key, val)) remove(model);
            }
        }

        // one-time multi-value rejection; all models who's attribute match any of the value are removed
        void rejectBy(const string &key, vector<string> &values){
            // we have to do this backwards! because every time you remove a model,
            // it messes with all the following index values
            for(int i=_models.size()-1; i>=0; i--){
                // get current model
                ModelClass* model = at(i);
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

        bool modelPassesActiveFilters(ModelClass* model){
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

        bool modelPassesMultiValueFilter(ModelClass *model, const string &attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return true;
            }

            return false;
        }

        bool modelPassesSingleValueFilter(ModelClass *model, const string &attr, const string &value){
            return model->get(attr) == value;
        }

        bool modelPassesActiveRejections(ModelClass* model){
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

        bool modelPassesMultiValueRejection(ModelClass *model, const string &attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return false;
            }

            return true;
        }

        bool modelPassesSingleValueRejection(ModelClass *model, const string &attr, string value){
            return model->get(attr) != value;
        }

    protected: // methods

        int indexByCid(const string &cid);
        string parseModelJsonValue(Json::Value &value);

        void registerSyncCallbacks(Collection<ModelClass> &otherCollection, bool _register = true){
            if(_register){
                ofAddListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
                ofAddListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
                ofAddListener(otherCollection.modelRemovedEvent, this, &Collection<ModelClass>::onSyncSourceModelRemoved);
                ofAddListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
            } else {
                ofRemoveListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
                ofRemoveListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
                ofRemoveListener(otherCollection.modelRemovedEvent, this, &Collection<ModelClass>::onSyncSourceModelRemoved);
                ofRemoveListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
            }
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
        ofEvent <ModelClass> modelAddedEvent;
        ofEvent <ModelClass> modelRemovedEvent;
        ofEvent <ModelClass> modelRejectedEvent;
        LambdaEvent <AttrChangeArgs> modelChangedEvent;
        ofEvent < Collection<ModelClass> > fifoEvent;

    protected: // attributes

        vector<ModelClass*> _models;
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
    _models.clear(); // just to be sure
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::initialize(vector< map<string, string> > &_data){
    for(int i=0; i<_data.size(); i++){
        // create a cloned copy of each model and add them without triggering modelAdded events
        add(new ModelClass(&_data[i]), false);
    }
    ofNotifyEvent(collectionInitializedEvent, this);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::Collection<ModelClass>::create(){
    auto ref = make_shared<ModelClass>();
    ref->setId(ofToString(mNextId));
    mNextId++;
    return ref;
}

template <class ModelClass>
bool ofxCMS::Collection<ModelClass>::add(ModelClass *model, bool notify){
    // What the hell are we supposed to do with this??
    if(model == NULL) return false;

    // apply active filters
    if(!modelPassesActiveFilters(model) || !modelPassesActiveRejections(model)){
        ofNotifyEvent(modelRejectedEvent, *model, this);
        return false;
    }

    // reached our limit, either remove first model, or reject this new model
    if(limitReached()){
        if(bFIFO){
            ofLog() << "Collection limit ("+ofToString(mLimit)+") reached, removing first model (FIFO)";
            ofNotifyEvent(fifoEvent, *this, this);
            remove(0);
        } else {
            ofLog() << "Collection limit ("+ofToString(mLimit)+") reached, can't add model (NO FIFO)";
            return false;
        }
    }

    // add to our collection
    _models.push_back(model);

    // add
    //registerModelCallbacks(model);
    this->modelChangedEvent.forward(model->attributeChangedEvent);

    // model->attributeChangedEvent += [this](ofxCMS::AttrChangeArgs& args) -> void {
    //     // if one of our models changed and with the new changes no longer
    //     // passes our active filters; remove it
    //     if(!modelPassesActiveFilters((ModelClass*)args.model)){
    //         remove((ModelClass*)args.model);
    //     }
    //
    //     // if one of our models changed and with the new changes no longer
    //     // passes our active rejections; remove it
    //     if(!modelPassesActiveRejections((ModelClass*)args.model)){
    //         remove((ModelClass*)args.model);
    //     }
    // }
    // };

    // // ofAddListener(model->beforeDestroyEvent, this, &Collection<ModelClass>::onModelDestroying);
    // ofAddListener(model->attributeChangedEvent, this, &Collection<ModelClass>::onModelAttributeChanged);

    //
    // // We have to use the Model& type here instead ModelClass& because all used Model types
    // // inherit from Model which has an ofEvent<Model> beforeDestroyEvent attribute which they all use...
    // template <class ModelClass>
    // void ofxCMS::Collection<ModelClass>::onModelAttributeChanged(AttrChangeArgs &args){
    //     // trigger a "forward" event; anybody can hook into this event to be notified
    //     // about changes in any of the collection's models
    //     ofNotifyEvent(modelChangedEvent, args, this);
    //
    //     // if one of our models changed and with the new changes no longer
    //     // passes our active filters; remove it
    //     if(!modelPassesActiveFilters((ModelClass*)args.model)){
    //         // TODO; ofNotifyEvent?
    //         remove((ModelClass*)args.model);
    //     }
    //
    //     // if one of our models changed and with the new changes no longer
    //     // passes our active rejections; remove it
    //     if(!modelPassesActiveRejections((ModelClass*)args.model)){
    //         // TODO; ofNotifyEvent?
    //         remove((ModelClass*)args.model);
    //     }
    // }





    // let's tell the world
    if(notify) ofNotifyEvent(modelAddedEvent, *model, this);

    // success!
    return true;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::remove(ModelClass *model, bool doDestroy){
    if(model == NULL){
		ofLogWarning("ofxCMS.Collection.remove") << "got NULL parameter";
		return NULL;
	}

    for(int i=0; i<this->_models.size(); i++){
        ModelClass* m = _models[i];

        if(m == NULL){ // this should be impossible but has been ocurring during debugging
            ofLogError("ofxCMS.Collection.remove") << "got impossible NULL from _models vector, local int i == " << i << ", _models.size() == " << _models.size();
            continue;
        }

        if(MODELS_MATCH(m, model)){
            return remove(i, doDestroy);
        }
    }

	ofLogWarning("ofxCMS.Collection.remove") << "couldn't find model";
    return NULL;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::remove(int index, bool doDestroy){
    ModelClass* model = at(index);

    if(model == NULL){
		ofLogWarning("ofxCMS.Collection.remove") << "couldn't find model with index: " << index;
		return NULL;
	}

    _models.erase(_models.begin() + index);
    ofNotifyEvent(modelRemovedEvent, *model, this);

    if(doDestroy && bDestroyOnRemove){
        ofLogNotice("ofxCMS.Collection.remove") << "destroying removed model with id: " << model->id();
        // destroy(model); // this will try to remove again, which isn't really a problem, just a bit inefficient
        model->destroy();
		delete model;
        return NULL;
    }

    return model;
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::destroy(ModelClass *model){
    if(model == NULL){
		ofLogWarning() << "ofxCMS::Collection::destroy(ModelClass*) - got NULL parameter";
		return;
	}

    remove(model, false /* just remove, no destroy */);
    model->destroy();
	delete model;
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::destroy(int idx){
    ModelClass* m = remove(idx, false /* just remove no destroy */);

	if(m){
		m->destroy();
		delete m;
		return;
	}

	ofLogWarning() << "ofxCMS::Collection::destroy(int) - couldn't find model";
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::destroyAll(){
    for(int i=_models.size()-1; i>=0; i--){
        destroy(i);
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::clear(){
    for(int i=_models.size()-1; i>=0; i--){
        remove(_models[i]);
    }
}

template <class ModelClass>
const vector<ModelClass*>& ofxCMS::Collection<ModelClass>::models(){
    return _models;
}

template <class ModelClass>
int ofxCMS::Collection<ModelClass>::indexByCid(const string &cid){
    for(int i=0; i<_models.size(); i++){
        if(_models[i]->cid() == cid)
            return i;
    }
    return -1;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::at(unsigned int idx){
	// if(idx < 0){ // this is impossible; idx is an UNSIGNED int
	// 	ofLogWarning() << "ofxCMS::Collection::at(unsigned int) - got negative index";
	// 	return;
	// }

	if(idx >= _models.size()){
		ofLogWarning() << "ofxCMS::Collection::at(unsigned int) - got invalid index";
		return NULL;
	}

    return _models[idx];
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::findByAttr(const string &attr, const string &value){
    for(int i=0; i<_models.size(); i++){
        if(_models[i]->get(attr) == value)
            return _models[i];
    }

    return NULL;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::findById(const string &_id){
    for(int i=0; i<_models.size(); i++){
        if(_models[i]->id() == _id)
            return _models[i];
    }

    return NULL;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::byCid(const string &_cid){
    int idx = indexByCid(_cid);
    return idx == -1 ? NULL : _models[idx];
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::filterBy(const string &key, const string &val){
    // we have to do this backwards! because every time you remove a model,
    // it messes with all the following index values
    for(int i=_models.size()-1; i>=0; i--){
        // get current model
        ModelClass* model = at(i);
        // remove it, if it doesn't meet the criteria
        if(!modelPassesSingleValueFilter(model, key, val)) remove(model);
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::filterBy(const string &key, vector<string> &values){
    // we have to do this backwards! because every time you remove a model,
    // it messes with all the following index values
    for(int i=_models.size()-1; i>=0; i--){
        // get current model
        ModelClass* model = at(i);
        // remove it, if it doesn't meet the criteria
        if(!modelPassesMultiValueFilter(model, key, values)) remove(model);
    }
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::destroyBy(const string &key, const string &value){
    // we have to do this backwards! because every time you remove a model,
    // it messes with all the following index values
    for(int i=_models.size()-1; i>=0; i--){
        // get current model
        ModelClass* model = at(i);
        // remove it, if it doesn't meet the criteria
        if(model->get(key) == value) destroy(model);
    }
}

template <class ModelClass>
bool ofxCMS::Collection<ModelClass>::parse(const string &jsonText, bool doRemove, bool doUpdate, bool doCreate){
    ofxJSONElement json;

    // try to parse json, abort if it fails
    if(!json.parse(jsonText)){
        ofLogWarning() << "Couldn't parse JSON:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return false;
    }

    // make sure we've got an array, we're a collection after all
    if(!json.isArray()){
        ofLogWarning() << "JSON not an array:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return false;
    }

    if(doRemove){
        // loop over all models that were already in the collection,
        // remove any model for which we can't find any record in the new json
        // IMPORTANT! Gotta start with the highest indexes first, because otherwise indexes
        // of higher-up models get messed up when removing models earlier in the list
        for(int i=_models.size()-1; i>=0; i--){
            // assume we'll have to remove the model as long as we haven't found a matching record in the json
            bool remove_model = true;

            // get the current model's id to match on
            string id = _models[i]->id();

            // loop over all items in the new json to see if there's a record with the same id
            for(int j=0; j<json.size(); j++){
                // if there's a record with this id, we don't have to do anything
                if(json[j]["_id"]["$oid"] == id)
                    remove_model = false;
            }

            // if remove_model us still true, this means that no records with a matching id was found,
            // meaning this in-memory record in no-longer was removed from the collection and we should drop it as well
            if(remove_model){
                destroy(_models[i]);
            }
        }
    }

    for(int i = 0; i < json.size(); i++) {
        ModelClass *existing = json[i]["_id"]["$oid"].isNull() ? NULL : findById(json[i]["_id"]["$oid"].asString());

        // found existing model with same id? update it by setting its json attribute
        if(existing && doUpdate){
            // let the Model attribute changed callbacks deal with further parsing
            parseModelJson(existing, ((ofxJSONElement)json[i]).getRawString(false));

        } else if(doCreate){
            // do an early limit check, to avoid unnecessary parsing
            if(limitReached() && !bFIFO){
                ofLog() << "Collection parsing: model skipped because limit reached (NO FIFO)";
            } else {
                //  not existing model found? Add a new one
                ModelClass *new_model = new ModelClass();

                parseModelJson(new_model, ((ofxJSONElement)json[i]).getRawString(false));
                // if we couldn't add this model to the collection
                // destroy the model, otherwise it's just hanging out in memory
                if(!add(new_model)){
                    delete new_model;
                }
            }
        }
    }

    ofLogVerbose() << "ofxCMS::Collection::parse() finished, number of models in collection: " << _models.size();
    ofNotifyEvent(collectionInitializedEvent, this);
    return true;
}

// for convenience
template <class ModelClass>
bool ofxCMS::Collection<ModelClass>::parse(const ofxJSONElement & node, bool doRemove, bool doUpdate, bool doCreate){
    if(node.type() == Json::nullValue) return false;

    // Can't figure out how to use this kinda object, so for now; let the text-based parse method deal with it
    // (meaning we'll convert back to text, and parse that to json again... yea...)
    if(node.type() == Json::stringValue) return parse(node.asString(), doRemove, doUpdate, doCreate);
    return parse(node.getRawString(), doRemove, doUpdate, doCreate);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::parseModelJson(ModelClass *model, const string &jsonText){
    ofxJSONElement doc;

    if(!doc.parse(jsonText)){
        ofLogWarning() << "ofxCMS::Collection::parseModelJson() - couldn't parse json:\n-- JSON start --\n" << jsonText << "\n-- JSON end --";
        return;
    }

    vector<string> attrs = doc.getMemberNames();
    for(int i=0; i<attrs.size(); i++){
        model->set(attrs[i], parseModelJsonValue(doc[attrs[i]]));
    }
}

template <class ModelClass>
string ofxCMS::Collection<ModelClass>::parseModelJsonValue(Json::Value &value){
    //    return value.asString();
    if(value.isObject() && value.isMember("$oid")) return value["$oid"].asString();
    if(value.isObject() && value.isMember("$date")) return ofToString(value["$date"]);
    if(value.isObject()) return ((ofxJSONElement)value).getRawString(false);
    // here's a real clumsy way of removing leading and trailing white-space and double-quotes;
    string val = ofToString(value);
    // trim left
    val.erase(0, val.find_first_not_of(" \n\r\t"));
    // trim right
    val.erase(val.find_last_not_of(" \n\r\t")+1);
    // trim leading quote
    if(val.find('"') == 0) val.erase(0, 1);
    // trim trailing quote
    if(val.rfind('"') == val.length()-1) val.erase(val.length()-1);

//        while(unsigned i = val.find("\\\"") != string::npos)
//            val.erase(i, 1);

    return val;
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::previous(ModelClass* model){
    int idx = indexByCid(model->cid());
    if(idx == -1) return NULL;
    return at((idx-1) % models().size());
}

template <class ModelClass>
ModelClass* ofxCMS::Collection<ModelClass>::next(ModelClass* model){
    int idx = indexByCid(model->cid());
    if(idx == -1) return NULL;
    return at((idx+1) % models().size());
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
    if(idx != INVALID_INDEX) remove(idx/*, false /* just remove, don't destroy? Syncing collections, probably shouldn't destroy on remove anyway... */);
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
        ofLogWarning() << "ofxCMS::Collection::onSyncSourceModelChanged(AttrChangeArgs &) - got NULL model";
        return;
    }

    //
    // Use active filters and rejections to re-evaluate if the syncSource's model
    // should or should not be in our collection after the change to its properties
    //

    // see if the model passes our active filter and rejection rules
    bool pass = modelPassesActiveFilters((ModelClass*)args.model) && modelPassesActiveRejections((ModelClass*)args.model);

    if(this->has((ModelClass*)args.model)){
        // already in our collection;
        // if after the change the model does NOT pass our filters; remove it
        if(!pass)
            remove((ModelClass*)args.model);

        return;
    }

    // not yet in our collection?
    // if after the change the model DOES pass our filters; add it
    if(pass)
        add((ModelClass*)args.model);
}

template <class ModelClass>
void ofxCMS::Collection<ModelClass>::onSyncSourceDestroying(Collection<ModelClass> &syncSourceCollection){
    stopSyncing();
}
