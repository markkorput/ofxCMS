//
//  CMSCollection.h
//  ofxCMS
//
//  Created by Mark van de Korput on 01/05/17.
//
//

#pragma once

#include "lib/Middleware.h"

// BaseCollection is a "core-essential" base class for the main Collection class.
// BaseCollection only implement the bare basics of creating, adding, update and removing items.

namespace ofxCMS {
        template<class ModelClass>
        class BaseCollection{

        public: // types & constants

            const static unsigned int NO_LIMIT = 0;
            const static unsigned int INVALID_INDEX = -1;

            typedef FUNCTION<void(shared_ptr<ModelClass>)> ModelRefFunc;

            // used in attributeChangeEvent notifications
            struct AttrChangeArgs {
                shared_ptr<ModelClass> modelRef;
                string attr;
                string value;
            };

            class Modification {
                public:
                    shared_ptr<ModelClass> addRef;
                    unsigned int removeCid;
                    bool notify;
                    Modification() : addRef(nullptr), removeCid(ModelClass::INVALID_CID), notify(true){}
                    Modification(shared_ptr<ModelClass> ref, bool _notify=true) : addRef(ref), removeCid(ModelClass::INVALID_CID), notify(_notify){}
                    Modification(int cid, bool _notify=true) : addRef(nullptr), removeCid(cid), notify(_notify){}
            };

        public: // methods

            BaseCollection() : nestedRefIteratorsCount(0){}
            ~BaseCollection(){ destroy(); }

            void setup(vector< map<string, string> > &_data);
            void destroy();

            // CRUD - Create
            shared_ptr<ModelClass> create();
            void add(shared_ptr<ModelClass> modelRef, bool notify=true);
            void initialize(vector<map<string, string>>& _data);

            // CRUD - Read
            const vector<shared_ptr<ModelClass>> &models(){ return modelRefs; }
            shared_ptr<ModelClass> at(unsigned int idx);
            shared_ptr<ModelClass> find(unsigned int cid){ return findByCid(cid); }
            shared_ptr<ModelClass> findByCid(unsigned int cid);

            unsigned int size(){ return modelRefs.size(); }
            bool has(shared_ptr<ModelClass> model){ return indexOfCid(model->cid()) != INVALID_INDEX; }
            bool has(int cid){ return indexOfCid(cid) != INVALID_INDEX; }
            int randomIndex(){ return size() == 0 ? INVALID_INDEX : floor(ofRandom(size())); }
            shared_ptr<ModelClass> random(){ return size() == 0 ? nullptr : at(randomIndex()); }
            shared_ptr<ModelClass> previous(shared_ptr<ModelClass> model, bool wrap=false);
            shared_ptr<ModelClass> next(shared_ptr<ModelClass> model, bool wrap=false);

            // CRUD - "update"
            void each(ModelRefFunc func);

            // CRUD - Delete
            shared_ptr<ModelClass> remove(shared_ptr<ModelClass> model, bool notify=true);
            shared_ptr<ModelClass> removeByCid(int cid, bool notify=true);
            shared_ptr<ModelClass> remove(unsigned int index, bool notify=true);

        private: // methods

            int indexOfCid(unsigned int cid);
            unsigned int nextCid(){ return ModelClass::nextCid; }
            void setNextCid(unsigned int newNextCid){ ModelClass::nextCid = newNextCid; }
            bool isIterating() const { return nestedRefIteratorsCount > 0; }

        public: // events

            Middleware<ofxCMS::Model> beforeAdd;
            LambdaEvent<ModelClass> modelAddedEvent;
            LambdaEvent<BaseCollection<ModelClass>> initializeEvent;
            LambdaEvent<ModelClass> modelChangeEvent;
            LambdaEvent<AttrChangeArgs> attributeChangeEvent;
            LambdaEvent<ModelClass> modelRemoveEvent;

        private: // attributes

            // unsigned int mNextId;
            std::vector<shared_ptr<ModelClass>> modelRefs;
            unsigned int nestedRefIteratorsCount;
            std::vector<shared_ptr<Modification>> operationsQueue;
    };
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::destroy(){
    if(isIterating()){ // unlikely
        ofLogWarning() << "shouldn't destroy collection while it's being iterated over, aborting destroy";
        return;
    }

    for(int i=modelRefs.size()-1; i>=0; i--){
        remove(i, false /* don't notify */);
    }

    modelRefs.clear();
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::create(){
    // create instance with auto-incremented ID
    auto ref = make_shared<ModelClass>();
    // add to our collection and return
    add(ref);
    return ref;
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::add(shared_ptr<ModelClass> modelRef, bool notify){
    // vector being iterated over? schedule removal operation for when iteration is done
    if(isIterating()){
        operationsQueue.push_back(make_shared<Modification>(modelRef, notify));
        return;
    }

    if(modelRef == nullptr){
        // What the hell are we supposed to do with this??
        ofLogWarning() << "got nullptr model to add to collection";
        return;
    }

    // make sure we have a valid CID
    if(modelRef->cid() == ModelClass::INVALID_CID){
        // ofLogNotice() << "nextId: " << nextCid();
        modelRef->setCid(nextCid());
        setNextCid(nextCid()+1);
    } else if(modelRef->cid() >= nextCid()){
        ofLogWarning() << "TODO: check if model with this cid doesn't already exist";
        setNextCid(modelRef->cid() + 1);
    }

    if(!beforeAdd(*modelRef.get()))
        return;

    // add to our collection
    modelRefs.push_back(modelRef);

    // register callbacks (unregistered in .remove)
    this->modelChangeEvent.forward(modelRef->changeEvent);

    modelRef->attributeChangeEvent.addListener([this](ofxCMS::Model::AttrChangeArgs& args) -> void {
        // turn regular pointer into a shared_ptr (Ref) by looking it up in our internal ref list
        auto changedModelRef = this->findByCid(args.model->cid());

        if(changedModelRef == nullptr){
            ofLogWarning() << "go attribute change from unknown model";
            return;
        }

        AttrChangeArgs collectionArgs;
        collectionArgs.modelRef = changedModelRef;
        collectionArgs.attr = args.attr;
        collectionArgs.value = args.value;
        this->attributeChangeEvent.notifyListeners(collectionArgs);
    }, this);

    // let's tell the world
    if(notify)
        ofNotifyEvent(modelAddedEvent, *(modelRef.get()), this);

    // success!
    return;
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::initialize(vector<map<string, string>>& _data){
    // remove all existing data
    destroy();

    // create and add models
    for(int i=0; i<_data.size(); i++){
        auto newModel = make_shared<ModelClass>();
        newModel->set(_data[i], false /* no individual model notification */);
        add(newModel);
    }

    // notify
    initializeEvent.notifyListeners(*this);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::previous(shared_ptr<ModelClass> model, bool wrap){
    int idx = indexOfCid(model->cid());

    if(idx == INVALID_INDEX)
        return nullptr;

    if(idx > 0)
        return at(idx-1);

    if(wrap)
        return at(size()-1);

    return nullptr;
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::next(shared_ptr<ModelClass> model, bool wrap){
    int idx = indexOfCid(model->cid());

    if(idx == INVALID_INDEX)
        return nullptr;

    if(idx < size()-1)
        return at(idx+1);

    if(wrap)
        return at(0);

    return nullptr;
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::at(unsigned int idx){
    if(idx >= size()){
        ofLogWarning() << "got invalid index";
        return nullptr;
    }

    return modelRefs.at(idx);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::findByCid(unsigned int cid){
    int idx = indexOfCid(cid);
    if(idx == INVALID_INDEX)
        return nullptr;
    return at(idx);
}


template <class ModelClass>
int ofxCMS::BaseCollection<ModelClass>::indexOfCid(unsigned int cid){
    int idx=0;

    for(auto modelRef : modelRefs){
        if(modelRef->cid() == cid)
            return idx;
        idx++;
    }

    return INVALID_INDEX;
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::each(ModelRefFunc func){
    nestedRefIteratorsCount++;

    for(auto modelRef : modelRefs){
        func(modelRef);
    }

    nestedRefIteratorsCount--;

    // still (recursively) iterating over our vector? skip processing opereations queue
    if(isIterating())
        return;

    // after we're done iterating, we should process any items
    // accumulated in the vector modificaton queue
    for(auto modification : operationsQueue){
        if(modification->addRef){
            add(modification->addRef, modification->notify);
        } else {
            removeByCid(modification->removeCid, modification->notify);
        }
    }

    operationsQueue.clear();
}

template <class ModelClass>
shared_ptr<ModelClass>  ofxCMS::BaseCollection<ModelClass>::remove(shared_ptr<ModelClass> modelRef, bool notify){
    if(modelRef == nullptr){
        ofLogWarning() << "got NULL parameter";
        return nullptr;
    }

    // find index and remove by index
    return removeByCid(modelRef->cid());
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::removeByCid(int cid, bool notify){
    // vector being iterated over? schedule removal operation for when iteration is done
    if(isIterating()){
        operationsQueue.push_back(make_shared<Modification>(cid, notify));
        return nullptr;
    }

    int idx = indexOfCid(cid);

    if(idx == INVALID_INDEX){
        ofLogWarning() << "could not find model to remove by cid";
        return nullptr;
    }

    // find
    auto modelRef = at(idx);

    // remove callbacks
    modelRef->attributeChangeEvent.removeListeners(this);
    this->modelChangeEvent.stopForward(modelRef->changeEvent);

    // remove
    modelRefs.erase(modelRefs.begin() + idx);

    // notify
    if(notify)
        modelRemoveEvent.notifyListeners(*modelRef.get());

    // return removed instance
    return modelRef;
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::remove(unsigned int index, bool notify){
    auto modelRef = at(index);

    // check
    if(modelRef == nullptr){
        ofLogWarning() << "couldn't find model with index: " << index;
        return nullptr;
    }

    // invoke main remove routine
    return remove(modelRef->cid(), notify);
}
