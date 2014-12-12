//
//  CMSCollection.h
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#ifndef __ofxCMS__CMSCollection__
#define __ofxCMS__CMSCollection__

#include "ofMain.h"
#include "ofxJSONElement.h"

// Even though CMS::Collection is a template class,
// it does assume that any used model-type inherits from CMS::Model
#include "CMSModel.h"

namespace CMS {

    // Collection class that manages a collections of Models,
    // kinda based on the Backbone.js Collection
    template<class ModelClass>
    class Collection{

    public: // methods

        const static int NO_LIMIT = -1;
        const static int INVALID_INDEX = -1;

        Collection() : _syncSource(NULL), mLimit(NO_LIMIT), bFIFO(false), bDestroyOnRemove(false){}
        ~Collection();

        void initialize(vector< map<string, string> > &_data);

        bool add(ModelClass *model, bool notify = true);
        ModelClass* remove(ModelClass *model, bool justRemove = false);
        ModelClass* remove(int index, bool justRemove = false);
        void destroy(int index);
        void destroy(ModelClass *model);
        void destroyBy(string key, string value);
        void clear();
        void destroyAll();

        const vector<ModelClass*> &models();
        unsigned int count(){ return _models.size(); }

        ModelClass* at(unsigned int idx);
        ModelClass* findByAttr(string attr, string value);
        ModelClass* findById(string _id);
        ModelClass* byCid(string cid);
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
                if(_models[i] == m)
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

        bool parse(string jsonText, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
        bool parse(const ofxJSONElement & node, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
        void parseModelJson(ModelClass *model, string jsonText);

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
        void filterBy(string key, string val);

        // One-time filter: only keep models with any of the specified values for a specific key
        void filterBy(string key, vector<string> &values);

        // Active Filter: only keep models with a specific key-value combination
        // and also apply this filter when new models are added
        void filtersBy(string attr, string value){
            // apply filter on current collection
            filterBy(attr, value);
            // save filter to apply to newly added models
            filterValues[attr] = value;
        }

        // Active Filter: only keep models with any of the specified values for a specific key
        // and also apply this filter when new models are added
        void filtersBy(string attr, vector<string> &values){
            // apply filter on current collection
            filterBy(attr, values);
            // save filter to apply to newly added models
            filterVectors[attr] = values;
        }

        // One-time filter: rejection only keep models that DO NOT have a specific key-value combination
        void rejectBy(string key, string val){
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
        void rejectBy(string key, vector<string> &values){
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
        void rejectsBy(string attr, string value){
            // apply filter on current collection
            rejectBy(attr, value);
            // save filter to apply to newly added models
            rejectValues[attr] = value;
        }

        // Active Filter: only keep models without any of the specified values for a specific key
        // and also apply this filter when new models are added
        void rejectsBy(string attr, vector<string> &values){
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

        void removeFilter(string attr, bool resync = true){
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

        bool modelPassesMultiValueFilter(ModelClass *model, string attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return true;
            }
            
            return false;
        }
        
        bool modelPassesSingleValueFilter(ModelClass *model, string attr, string value){
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

        bool modelPassesMultiValueRejection(ModelClass *model, string attr, vector<string> &values){
            for(int i=0; i<values.size(); i++){
                // passes if it has one of the specified values
                if(model->get(attr) == values[i]) return false;
            }

            return true;
        }

        bool modelPassesSingleValueRejection(ModelClass *model, string attr, string value){
            return model->get(attr) != value;
        }
        
    protected: // methods

        int indexByCid(string cid);
        string parseModelJsonValue(Json::Value &value);

        void registerSyncCallbacks(Collection<ModelClass> &otherCollection, bool _register = true){
            if(_register){
                ofAddListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
                ofAddListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
                // ofLogWarning() << "TODO: register onDestroy callback as well, to unregister when sync source destroys";
                ofAddListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
            } else {
                ofRemoveListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
                ofRemoveListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
                ofRemoveListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
            }
        }

        void registerModelCallbacks(ModelClass* model, bool _register = true){
            if(_register){
                // when a models (self-)destructs, we gotta remove it from our collection,
                // otherwise we end up with invalid pointers
                ofAddListener(model->beforeDestroyEvent, this, &CMS::Collection<ModelClass>::onModelDestroying);
                ofAddListener(model->attributeChangedEvent, this, &Collection<ModelClass>::onModelAttributeChanged);
            } else {
                ofRemoveListener(model->attributeChangedEvent, this, &Collection<ModelClass>::onModelAttributeChanged);
                ofRemoveListener(model->beforeDestroyEvent, this, &CMS::Collection<ModelClass>::onModelDestroying);
            }
        }

    protected: // callbacks
        
        // NOTE: Model& type, not ModelClass& (see comments at implementation)
        void onModelDestroying(Model& model);
        void onSyncSourceModelAdded(ModelClass &m);
        void onSyncSourceModelChanged(AttrChangeArgs &args);
        void onModelAttributeChanged(AttrChangeArgs &args);
        void onSyncSourceDestroying(Collection<ModelClass> &syncSourceCollection);
        
    public: // events
        
        ofEvent <void> collectionInitializedEvent;
        ofEvent < Collection<ModelClass> > collectionDestroyingEvent;
        ofEvent <ModelClass> modelAddedEvent;
        ofEvent <ModelClass> modelRemovedEvent;
        ofEvent <ModelClass> modelRejectedEvent;
        ofEvent <AttrChangeArgs> modelChangedEvent;
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

    }; // class Collection


    // TEMPLATE CLASS IMPLEMENTATION (there's no .cpp files) //


    template <class ModelClass>
    CMS::Collection<ModelClass>::~Collection(){
        ofNotifyEvent(collectionDestroyingEvent, *this, this);

        for(int i=0; i<_models.size(); i++){
            remove(_models[i]);
            // delete _models[i]; // destructing collections don't necessarily detroy their content
        }

        //_models.clear();

        stopSyncing();
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::initialize(vector< map<string, string> > &_data){
        for(int i=0; i<_data.size(); i++){
            // create a cloned copy of each model and add them without triggering modelAdded events
            add(new ModelClass(&_data[i]), false);
        }
        ofNotifyEvent(collectionInitializedEvent, this);
    }

    template <class ModelClass>
    bool CMS::Collection<ModelClass>::add(ModelClass *model, bool notify){
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

        registerModelCallbacks(model);

        // let's tell the world
        if(notify) ofNotifyEvent(modelAddedEvent, *model, this);

        // success!
        return true;
    }

    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::remove(ModelClass *model, bool justRemove){
        if(model == NULL) return NULL;

//        // get specified model's index
//        if(this->_models.size() > 900){
//            ofLog() << "this is crazy: " << this->_models.size();
//            return model;
//        }
//        ofLog() << "_models size: " << this->_models.size();

        for(int i=this->_models.size()-1; i>=0; i--){
            if(this->_models[i] == model || _models[i]->cid() == model->cid()){
                return remove(i, justRemove);
            }
        }

        return NULL;
    }

    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::remove(int index, bool justRemove){
        ModelClass* model = at(index);
        if(model == NULL) return NULL;

        registerModelCallbacks(model, false);
        _models.erase(_models.begin() + index);
        
        if(justRemove) return model;

        // todo; this should probably happen even is justRemove == true
        ofNotifyEvent(modelRemovedEvent, *model, this);

        if(bDestroyOnRemove){
            ofLog() << "Destroying removed model (id="+model->id()+", bDestroyOnRemove=true)";
            // destroy(model); // this will try to remove again, which isn't really a problem, just a bit inefficient
            // delete model;
            model->destroy();
            return NULL;
        }

        return model;
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::destroy(ModelClass *model){
        if(model == NULL) return;
        remove(model, true /* just remove, no destroy */);
        // delete model;
        model->destroy();
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::destroy(int idx){
        ModelClass* m = remove(idx, true /* just remove no destroy */);
        // delete m;
        m->destroy();
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::destroyAll(){
        for(int i=_models.size()-1; i>=0; i--){
            // delete _models[i];
            destroy(i);
        }
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::clear(){
        for(int i=_models.size()-1; i>=0; i--){
            remove(_models[i]);
        }
    }
    
    template <class ModelClass>
    const vector<ModelClass*> &CMS::Collection<ModelClass>::models(){
        return _models;
    }
    
    template <class ModelClass>
    int CMS::Collection<ModelClass>::indexByCid(string cid){
        for(int i=0; i<_models.size(); i++){
            if(_models[i]->cid() == cid)
                return i;
        }
        return -1;
    }
    
    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::at(unsigned int idx){
        return _models[idx];
    }
    
    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::findByAttr(string attr, string value){
        for(int i=0; i<_models.size(); i++){
            if(_models[i]->get(attr) == value)
                return _models[i];
        }
        
        return NULL;
    }

    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::findById(string _id){
        for(int i=0; i<_models.size(); i++){
            if(_models[i]->id() == _id)
                return _models[i];
        }
        
        return NULL;
    }

    template <class ModelClass>
    ModelClass* CMS::Collection<ModelClass>::byCid(string _cid){
        int idx = indexByCid(_cid);
        return idx == -1 ? NULL : _models[idx];
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::filterBy(string key, string val){
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
    void CMS::Collection<ModelClass>::filterBy(string key, vector<string> &values){
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
    void CMS::Collection<ModelClass>::destroyBy(string key, string value){
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
    bool Collection<ModelClass>::parse(string jsonText, bool doRemove, bool doUpdate, bool doCreate){
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

        ofLogVerbose() << "CMS::Collection::parse() finished, number of models in collection: " << _models.size();
        ofNotifyEvent(collectionInitializedEvent, this);
        return true;
    }

    // for convenience
    template <class ModelClass>
    bool Collection<ModelClass>::parse(const ofxJSONElement & node, bool doRemove, bool doUpdate, bool doCreate){
        if(node.type() == Json::nullValue) return false;
        
        // Can't figure out how to use this kinda object, so for now; let the text-based parse method deal with it
        // (meaning we'll convert back to text, and parse that to json again... yea...)
        if(node.type() == Json::stringValue) return parse(node.asString(), doRemove, doUpdate, doCreate);
        return parse(node.getRawString(), doRemove, doUpdate, doCreate);
    }

    template <class ModelClass>
    void Collection<ModelClass>::parseModelJson(ModelClass *model, string jsonText){
        ofxJSONElement doc;

        if(!doc.parse(jsonText)){
            ofLogWarning() << "CMS::Collection::parseModelJson() - couldn't parse json:\n-- JSON start --\n" << jsonText << "\n-- JSON end --";
            return;
        }

        vector<string> attrs = doc.getMemberNames();
        for(int i=0; i<attrs.size(); i++){
            model->set(attrs[i], parseModelJsonValue(doc[attrs[i]]));
        }
    }

    template <class ModelClass>
    string Collection<ModelClass>::parseModelJsonValue(Json::Value &value){
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
    ModelClass* Collection<ModelClass>::previous(ModelClass* model){
        int idx = indexByCid(model->cid());
        if(idx == -1) return NULL;
        return at((idx-1) % models().size());
    }

    template <class ModelClass>
    ModelClass* Collection<ModelClass>::next(ModelClass* model){
        int idx = indexByCid(model->cid());
        if(idx == -1) return NULL;
        return at((idx+1) % models().size());
    }

    template <class ModelClass>
    void Collection<ModelClass>::clone(Collection<ModelClass> &source){
        clear(); // triggers modelRemovedEvents for each model
        for(int i=0; i<source.models().size(); i++){
            add(source.at(i)); // trigges modelAddedEvents
        }
    }

    template <class ModelClass>
    void Collection<ModelClass>::syncsFrom(Collection<ModelClass> &collection, bool clearFirst){
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
    void Collection<ModelClass>::stopSyncing(){
        if(_syncSource){
            registerSyncCallbacks(*_syncSource, false);
            _syncSource = NULL;
        }
    }

    // We have to use the Model& type here instead ModelClass& because all used Model types
    // inherit from Model which has an ofEvent<Model> beforeDestroyEvent attribute which they all use...
    template <class ModelClass>
    void Collection<ModelClass>::onModelDestroying(Model& model){
        remove((ModelClass*)&model, true /* just remove */);
    }

    // We have to use the Model& type here instead ModelClass& because all used Model types
    // inherit from Model which has an ofEvent<Model> beforeDestroyEvent attribute which they all use...
    template <class ModelClass>
    void Collection<ModelClass>::onModelAttributeChanged(AttrChangeArgs &args){
        // trigger a "forward" event; anybody can hook into this event to be notified
        // about changes in any of the collection's models
        ofNotifyEvent(modelChangedEvent, args, this);

        // if one of our models changed and with the new changes no longer
        // passes our active filters; remove it
        if(!modelPassesActiveFilters((ModelClass*)args.model)){
            remove((ModelClass*)args.model);
        }
    }

    template <class ModelClass>
    void Collection<ModelClass>::onSyncSourceModelAdded(ModelClass &m){
        // if our sync source gets a new model, we follow...
        add(&m);
    }

    template <class ModelClass>
    void Collection<ModelClass>::onSyncSourceModelChanged(AttrChangeArgs &args){
        // if our sync source's model isn't in our collection, but DOES pass our filters, add it
        if(args.model == NULL) return;
        if(this->findById(args.model->id()) != NULL) return; // already in our collection
        if(modelPassesActiveFilters((ModelClass*)args.model)) add((ModelClass*)args.model);
    }
    
    template <class ModelClass>
    void Collection<ModelClass>::onSyncSourceDestroying(Collection<ModelClass> &syncSourceCollection){
        stopSyncing();
    }
    
}; // namespace CMS



#endif /* defined(__ofxCMS__CMSCollection__) */
