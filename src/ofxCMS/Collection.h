#pragma once

#include "BaseCollection.h"
#include "CollectionLimit.h"
#include "CollectionSync.h"
#include "CollectionFilter.h"
#ifdef OFXCMS_JSON
    #include "JsonParser.h"
#endif

namespace ofxCMS {
    template<class ModelClass>
    class Collection : public BaseCollection<ModelClass> {

    public: // types & constants
        typedef FUNCTION<bool(ModelClass&)> FilterFunctor;

    public: // methods
        Collection(){}

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

        void sync(shared_ptr<Collection<ModelClass>> other, bool active=true);
        void stopSync(shared_ptr<Collection<ModelClass>> other);

        void filter(const string& attr, const string& value, bool active=true);
        void reject(const string& attr, const string& value, bool active=true);
        void filter(FilterFunctor func, bool active=true);

#ifdef OFXCMS_JSON
        bool loadJsonFromFile(const string& filename);
#endif

    private: // attributes
        CollectionLimit<ModelClass> collectionLimit;
        std::vector<shared_ptr<CollectionSync<ModelClass>>> collectionSyncs;
        std::vector<shared_ptr<CollectionFilter<ModelClass>>> collectionFilters;
    };
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::limit(unsigned int amount){
    collectionLimit.setup(this, amount);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::sync(shared_ptr<Collection<ModelClass>> other, bool active){
    auto sync = make_shared<CollectionSync<ModelClass>>();
    sync->setup(this, other);

    // if active safe pointer so it doesn't auto-destruct
    if(active)
        collectionSyncs.push_back(sync);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::stopSync(shared_ptr<Collection<ModelClass>> other){
    for(auto it = collectionSyncs.begin(); it != collectionSyncs.end(); it++){
        if(it->getSource() == other){
            collectionSyncs.erase(it);
            return;
        }
    }

    ofLogWarning() << "Could not source to stop syncing from";
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::filter(const string& attr, const string& value, bool active){
    auto filter = make_shared<CollectionFilter<ModelClass>>();
    filter->setup(this, attr, value);

    // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    if(active)
        collectionFilters.push_back(filter);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::reject(const string& attr, const string& value, bool active){
    auto filter = make_shared<CollectionFilter<ModelClass>>();
    filter->setup(this, attr, value, false);

    // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    if(active)
        collectionFilters.push_back(filter);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::filter(FilterFunctor func, bool active){
    auto filter = make_shared<CollectionFilter<ModelClass>>();
    filter->setup(this, func);

    // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    if(active)
        collectionFilters.push_back(filter);
}

#ifdef OFXCMS_JSON
template<class ModelClass>
bool ofxCMS::Collection<ModelClass>::loadJsonFromFile(const string& filename){
    JsonParser<ModelClass> jsonParser;
    jsonParser.setup(this, filename);
    jsonParser.load();
}
#endif
