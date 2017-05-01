//
//  CMSCollection.h
//  ofxCMS
//
//  Created by Mark van de Korput on 01/05/17.
//
//

#pragma once

// BaseCollection is a "core-essential" base class for the main Collection class.
// BaseCollection only implement the bare basics of creating, adding, update and removing items.

namespace ofxCMS {
        template<class ModelClass>
        class BaseCollection{

        public: // types & constants

            const static unsigned int NO_LIMIT = 0;
            const static unsigned int INVALID_INDEX = -1;
            const static int INVALID_CID = -1;

            // used in attributeChangeEvent notifications
            struct AttrChangeArgs {
                shared_ptr<ModelClass> modelRef;
                string attr;
                string value;
            };

        public: // methods

            BaseCollection() : mNextId(1){}
            ~BaseCollection(){ destroy(); }

            void setup(vector< map<string, string> > &_data);
            void destroy();

            // CRUD - Create
            shared_ptr<ModelClass> create();
            void add(shared_ptr<ModelClass> modelRef, bool notify=true);

            unsigned int size(){ return modelRefs.size(); }
            int randomIndex(){ return size() == 0 ? INVALID_INDEX : floor(ofRandom(size())); }
            shared_ptr<ModelClass> random(){ return size() == 0 ? nullptr : at(randomIndex()); }

            // CRUD - Read
            const vector<shared_ptr<ModelClass>> &models(){ return modelRefs; }
            shared_ptr<ModelClass> at(unsigned int idx);
            shared_ptr<ModelClass> find(int cid){ return findByCid(cid); }
            shared_ptr<ModelClass> findByCid(int cid);

            // CRUD - Delete
            shared_ptr<ModelClass> remove(shared_ptr<ModelClass> model, bool notify=true);
            shared_ptr<ModelClass> remove(int index, bool notify=true);

        private: // methods

            int indexOfCid(int cid);

        public: // events

            LambdaEvent<ModelClass> modelAddedEvent;
            LambdaEvent<ModelClass> modelChangedEvent;
            LambdaEvent<AttrChangeArgs> attributeChangedEvent;
            LambdaEvent <ModelClass> modelRemovedEvent;

        private: // attributes

            unsigned int mNextId;
            vector<shared_ptr<ModelClass>> modelRefs;

    };
}

template <class ModelClass>
void ofxCMS::BaseCollection<ModelClass>::destroy(){
    for(int i=modelRefs.size()-1; i>=0; i--){
        remove(i);
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
    if(modelRef == nullptr){
        // What the hell are we supposed to do with this??
        ofLogWarning() << "got nullptr model to add to collection";
        return;
    }

    // make sure we have a valid CID
    if(modelRef->cid() == INVALID_CID){
        ofLogNotice() << "nextId: " << mNextId;
        modelRef->setCid(mNextId);
        mNextId++;
    } else if(modelRef->cid() >= mNextId){
        ofLogWarning() << "TODO: check if model with this cid doesn't already exist";
        mNextId = modelRef->cid() + 1;
    }

    // add to our collection
    modelRefs.push_back(modelRef);

    // add
    //registerModelCallbacks(model);
    this->modelChangedEvent.forward(modelRef->changeEvent);

    modelRef->attributeChangedEvent.addListener([this](ofxCMS::Model::AttrChangeArgs& args) -> void {
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
        this->attributeChangedEvent.notifyListeners(collectionArgs);
    }, this);

    // let's tell the world
    if(notify)
        ofNotifyEvent(modelAddedEvent, *(modelRef.get()), this);

    // success!
    return;
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
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::findByCid(int cid){
    int idx = indexOfCid(cid);
    if(idx == INVALID_INDEX)
        return nullptr;
    return at(idx);
}


template <class ModelClass>
int ofxCMS::BaseCollection<ModelClass>::indexOfCid(int cid){
    int idx=0;

    for(auto modelRef : modelRefs){
        if(modelRef->cid() == cid)
            return idx;
        idx++;
    }

    return INVALID_INDEX;
}

template <class ModelClass>
shared_ptr<ModelClass>  ofxCMS::BaseCollection<ModelClass>::remove(shared_ptr<ModelClass> model, bool notify){
    if(model == nullptr){
        ofLogWarning() << "got NULL parameter";
        return nullptr;
    }

    // find index and remove by index
    int i=0;
    for(auto modelRef : modelRefs){
        if(modelRef == nullptr){ // this should be impossible but has been ocurring during debugging
            ofLogError() << "got (impossible?) nullptr from models vector for index: " << i;
        } else if(model->equals(modelRef)){
            return remove(i, notify);
        }

        i++;
    }

    // didn't find it
	ofLogWarning() << "couldn't find specified model to remove";
    return nullptr;
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::BaseCollection<ModelClass>::remove(int index, bool notify){
    // find and remove
    auto modelRef = at(index);
    modelRefs.erase(modelRefs.begin() + index);

    // notify
    if(modelRef == nullptr){
        ofLogWarning() << "couldn't find model with index: " << index;
    } else if(notify){
        modelRemovedEvent.notifyListeners(*modelRef.get());
    }

    // return removed instance
    return modelRef;
}
