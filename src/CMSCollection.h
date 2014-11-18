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

        Collection() : _syncSource(NULL){}
        ~Collection();

        void initialize(vector< map<string, string> > &_data);

        void add(ModelClass *model, bool notify = true);
        void remove(ModelClass *model);
        void destroy(ModelClass *model);
        void destroyBy(string key, string value);
        void clear();
        void destroyAll();

        const vector<ModelClass*> &models();

        ModelClass* at(unsigned int idx);
        ModelClass* findByAttr(string attr, string value);
        ModelClass* findById(string _id);
        ModelClass* byCid(string cid);

        ModelClass* previous(ModelClass* model);
        ModelClass* next(ModelClass* model);

        void clone(Collection<ModelClass> &source);
        void syncsFrom(Collection<ModelClass> &collection);

    public: // parsing methods

        bool parse(string jsonText);
        void parse(const ofxJSONElement & node);
        void parseModelJson(ModelClass *model, string jsonText);

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

        void removeFilters(){
            filterValues.clear();
            filterVectors.clear();
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

    protected: // methods

        int indexByCid(string cid);
        string parseModelJsonValue(Json::Value &value);

        void registerSyncCallbacks(Collection<ModelClass> &otherCollection, bool _register = true){
            if(_register){
                ofAddListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
            } else {
                ofRemoveListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
            }
        }

    protected: // callbacks
        
        // NOTE: Model& type, not ModelClass& (see comments at implementation)
        void onModelDestroying(Model& model);
        void onSyncSourceModelAdded(ModelClass &m);
        
    public: // events
        
        ofEvent <void> collectionInitializedEvent;
        ofEvent <ModelClass> modelAddedEvent;
        ofEvent <ModelClass> modelRemovedEvent;
        ofEvent <ModelClass> modelRejectedEvent;

    protected: // attributes

        vector<ModelClass*> _models;
        Collection<ModelClass>* _syncSource;
        map<string, string> filterValues;
        map< string, vector<string> > filterVectors;

    }; // class Collection


    // TEMPLATE CLASS IMPLEMENTATION (there's no .cpp files) //


    template <class ModelClass>
    CMS::Collection<ModelClass>::~Collection(){
        for(int i=0; i<_models.size(); i++){
            remove(_models[i]);
            // delete _models[i]; // destructing collections don't necessarily detroy their content
        }

        _models.clear();

        if(_syncSource != NULL){
            registerSyncCallbacks(*_syncSource, false); // unregister callbacks
            _syncSource = NULL;
        }
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
    void CMS::Collection<ModelClass>::add(ModelClass *model, bool notify){
        // What the hell are we supposed to do with this??
        if(model == NULL) return;

        // apply active filters
        if(!modelPassesActiveFilters(model)){
            ofNotifyEvent(modelRejectedEvent, *model, this);
            return;
        }

        // add to our collection
        _models.push_back(model);

        // when a models (self-)destructs, we gotta remove it from our collection,
        // otherwise we end up with invalid pointers
        ofAddListener(model->beforeDestroyEvent, this, &CMS::Collection<ModelClass>::onModelDestroying);

        // let's tell the world
        if(notify) ofNotifyEvent(modelAddedEvent, *model, this);
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::remove(ModelClass *model){
        // get specified model's index
        for(int i=0; i<_models.size(); i++){
            if(_models[i] == model || _models[i]->cid() == model->cid()){
                // This event listener is added in our ::add() function
                ofRemoveListener(model->beforeDestroyEvent, this, &CMS::Collection<ModelClass>::onModelDestroying);

                _models.erase(_models.begin() + i);
                ofNotifyEvent(modelRemovedEvent, *model, this);

                // remove doesn't destroy!
                // destroy(model);
                
                // we're done, return
                return;
            }
        }

        // ofLogWarning() << "Couldn't remove model because it wasn't found in the _models collection";
    }
    
    template <class ModelClass>
    void CMS::Collection<ModelClass>::destroy(ModelClass *model){
        remove(model);
        if(model != NULL) delete model;
    }

    template <class ModelClass>
    void CMS::Collection<ModelClass>::destroyAll(){
        for(int i=0; i<_models.size(); i++){
            delete _models[i];
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
    bool Collection<ModelClass>::parse(string jsonText){
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

        for(int i = 0; i < json.size(); i++) {
            ModelClass *existing = findById(json[i]["_id"]["$oid"].asString());

            // found existing model with same id? update it by setting its json attribute
            if(existing){
                // let the Model attribute changed callbacks deal with further parsing
                parseModelJson(existing, ((ofxJSONElement)json[i]).getRawString(false));
            } else {
                //  not existing model found? Add a new one
                ModelClass *new_model = new ModelClass();
                parseModelJson(new_model, ((ofxJSONElement)json[i]).getRawString(false));
                add(new_model);
            }
        }

        ofLogVerbose() << "CMS::Collection::parse() finished, number of models in collection: " << _models.size();
        ofNotifyEvent(collectionInitializedEvent, this);
        return true;
    }


    // for convenience
    template <class ModelClass>
    void Collection<ModelClass>::parse(const ofxJSONElement & node){
        if(node.type() == Json::nullValue) return;
        
        // Can't figure out how to use this kinda object, so for now; let the text-based parse method deal with it
        // (meaning we'll convert back to text, and parse that to json again... yea...)
        if(node.type() == Json::stringValue) parse(node.asString());
        parse(node.getRawString());
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
    void Collection<ModelClass>::syncsFrom(Collection<ModelClass> &collection){
        // first, UNregister existing sync source callbacks
        if(_syncSource) registerSyncCallbacks(*_syncSource, false);
        _syncSource = &collection;   // we'll need this at destructor-time to unregister event callbacks
        clone(*_syncSource);        // first, clone the current content
        registerSyncCallbacks(*_syncSource); // second, register callbacks to stay up-to-date on later changes
    }

    // We have to use the Model& type here instead ModelClass& because all used Model types
    // inherit from Model which has an ofEvent<Model> beforeDestroyEvent attribute which they all use...
    template <class ModelClass>
    void Collection<ModelClass>::onModelDestroying(Model& model){
        remove((ModelClass*)&model);
    }

    template <class ModelClass>
    void Collection<ModelClass>::onSyncSourceModelAdded(ModelClass &m){
        // if our sync source gets a new model, we follow...
        add(&m);
    }

}; // namespace CMS



#endif /* defined(__ofxCMS__CMSCollection__) */
