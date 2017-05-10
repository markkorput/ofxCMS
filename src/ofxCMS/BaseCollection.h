//
//  CMSCollection.h
//  ofxCMS
//
//  Created by Mark van de Korput on 01/05/17.
//
//

#pragma once

#include "ofxLambdaEvent/LambdaEvent.h"
#include "ofxLambdaEvent/Middleware.h"

#define OFXCMS_INVALID_INDEX (-1)

// BaseCollection is a "core-essential" base class for the main Collection class.
// BaseCollection only implement the bare basics of creating, adding, update and removing items.

namespace ofxCMS {
        template<class ModelClass>
        class BaseCollection{

        public: // types & constants

            typedef FUNCTION<void(void)> LockFunctor;
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
                    CidType removeCid;
                    bool notify;
                    Modification() : addRef(nullptr), removeCid(OFXCMS_INVALID_CID), notify(true){}
                    Modification(shared_ptr<ModelClass> ref, bool _notify=true) : addRef(ref), removeCid(OFXCMS_INVALID_CID), notify(_notify){}
                    Modification(CidType cid, bool _notify=true) : addRef(nullptr), removeCid(cid), notify(_notify){}
            };

        public: // methods

            BaseCollection() : mNextCid(1), vectorLockCount(0){}
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
            shared_ptr<ModelClass> find(CidType cid){ return findByCid(cid); }
            shared_ptr<ModelClass> findByCid(CidType cid);
            shared_ptr<ModelClass> findById(const string& id);


            unsigned int size(){ return modelRefs.size(); }
            bool has(shared_ptr<ModelClass> model){ return indexOfCid(model->cid()) != OFXCMS_INVALID_INDEX; }
            bool has(CidType cid){ return indexOfCid(cid) != OFXCMS_INVALID_INDEX; }
            int randomIndex(){ return size() == 0 ? OFXCMS_INVALID_INDEX : floor(ofRandom(size())); }
            shared_ptr<ModelClass> random(){ return size() == 0 ? nullptr : at(randomIndex()); }
            shared_ptr<ModelClass> previous(shared_ptr<ModelClass> model, bool wrap=false);
            shared_ptr<ModelClass> next(shared_ptr<ModelClass> model, bool wrap=false);

            // CRUD - "update"
            void each(ModelRefFunc func);

            // CRUD - Delete
            shared_ptr<ModelClass> remove(shared_ptr<ModelClass> model, bool notify=true);
            shared_ptr<ModelClass> removeByCid(CidType cid, bool notify=true);
            shared_ptr<ModelClass> removeByIndex(unsigned int index, bool notify=true);

        private: // methods

            int indexOfCid(CidType cid);
            int indexOfId(const string& _id);
            CidType nextCid(){ ofLog() << "next CID"; return mNextCid; }
            void setNextCid(CidType newNextCid){ ofLog() << "SET next CID";  mNextCid = newNextCid; }
            bool isLocked() const { return vectorLockCount > 0; }
            void lock(LockFunctor func);

        public: // events

            Middleware<ModelClass> beforeAdd;
            LambdaEvent<ModelClass> modelAddedEvent;
            LambdaEvent<BaseCollection<ModelClass>> initializeEvent;
            LambdaEvent<AttrChangeArgs> attributeChangeEvent;
            LambdaEvent<ModelClass> modelRemoveEvent;

        private: // attributes

            unsigned int mNextCid;
            std::vector<shared_ptr<ModelClass>> modelRefs;
            unsigned int vectorLockCount;
            std::vector<shared_ptr<Modification>> operationsQueue;
    };
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::destroy(){
    if(isLocked()){ // unlikely
        ofLogWarning() << "shouldn't destroy collection while it's being iterated over, aborting destroy";
        return;
    }

    for(int i=modelRefs.size()-1; i>=0; i--){
        removeByIndex(i);
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
    if(isLocked()){
        operationsQueue.push_back(make_shared<Modification>(modelRef, notify));
        return;
    }

    if(modelRef == nullptr){
        // What the hell are we supposed to do with this??
        ofLogWarning() << "got nullptr model to add to collection";
        return;
    }

    // make sure we have a valid CID
    if(modelRef->cid() == OFXCMS_INVALID_CID){
        // ofLogNotice() << "nextId: " << nextCid();
        modelRef->setCid(modelRef.get());
        // setNextCid(nextCid()+1);
    } else if(modelRef->cid() != modelRef.get()) {
        ofLogWarning() << "TODO: model CID doesn't match pointer value. This OK?";
    }

    if(!beforeAdd(*modelRef.get()))
        return;

    // add to our collection
    modelRefs.push_back(modelRef);

    modelChangeEvent.forward(modelRef->changeEvent);

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
        newModel->set(_data[i]);
        add(newModel);
    }

    // notify
    initializeEvent.notifyListeners(*this);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::previous(shared_ptr<ModelClass> model, bool wrap){
    int idx = indexOfCid(model->cid());

    if(idx == OFXCMS_INVALID_INDEX)
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

    if(idx == OFXCMS_INVALID_INDEX)
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
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::findByCid(CidType cid){
    int idx = indexOfCid(cid);
    if(idx == OFXCMS_INVALID_INDEX)
        return nullptr;
    return at(idx);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::findById(const string& id){
    int idx = indexOfId(id);
    if(idx == OFXCMS_INVALID_INDEX)
        return nullptr;
    return at(idx);
}

template <class ModelClass>
int ofxCMS::BaseCollection<ModelClass>::indexOfCid(CidType cid){
    int idx=0;

    for(auto modelRef : modelRefs){
        if(modelRef->cid() == cid)
            return idx;
        idx++;
    }

    return OFXCMS_INVALID_INDEX;
}

template <class ModelClass>
int ofxCMS::BaseCollection<ModelClass>::indexOfId(const string& _id){
    int idx=0;

    for(auto modelRef : modelRefs){
        if(modelRef->getId() == _id)
            return idx;
        idx++;
    }

    return OFXCMS_INVALID_INDEX;
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::each(ModelRefFunc func){
    // when locked all add/remove operations to our internal modelRefs vector
    // are intercepted and queue for execution until after the lock is lifter,
    // so we can safely iterate over the vector and callbacks are free to call
    // our add/remove methods without causing errors
    lock([&](){
        for(auto modelRef : this->modelRefs){
            func(modelRef);
        }
    });
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
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::removeByCid(CidType cid, bool notify){
    // vector being iterated over? schedule removal operation for when iteration is done
    if(isLocked()){
        operationsQueue.push_back(make_shared<Modification>(cid, notify));
        return nullptr;
    }

    int idx = indexOfCid(cid);

    if(idx == OFXCMS_INVALID_INDEX){
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
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::removeByIndex(unsigned int index, bool notify){
    auto modelRef = at(index);

    // check
    if(modelRef == nullptr){
        ofLogWarning() << "couldn't find model with index: " << index;
        return nullptr;
    }

    // invoke main remove routine
    return removeByCid(modelRef->cid(), notify);
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::lock(LockFunctor func){
    vectorLockCount++;
    func();
    vectorLockCount--;

    // still (recursively) iterating over our vector? skip processing opereations queue
    if(isLocked())
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
