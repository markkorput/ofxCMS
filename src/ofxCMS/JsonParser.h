// public: // parsing methods
//
//     bool parse(const string &jsonText, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
//     bool parse(const ofxJSONElement & node, bool doRemove = true, bool doUpdate = true, bool doCreate = true);
//     void parseModelJson(shared_ptr<ModelClass> model, const string &jsonText);
//
//     // "merge" all models of another collection into our own collection.
//     // for each model in the other collection, it will try to find an existing
//     // model in our own collection (matching on model->id()). If found, that existing model
//     // is updated with the attributes of the other collection's model. If NOT found,
//     // a new model is created with the attributes of the other collection's model and
//     // added to our collection
//     void merge(Collection<ModelClass> &otherCollection){
//         // loop over other collection's models
//         for(int i=0; i<otherCollection.count(); i++){
//             auto otherModel = otherCollection.at(i);
//             if(otherModel == nullptr) continue;
//
//             // find existing matching model in our own collection
//             auto existing = this->findById(otherModel->id());
//             if(existing){
//                 // update existing model
//                 existing->set(otherModel->attributes());
//                 // done
//                 continue;
//             }
//
//             // no existing model found, create new model
//             auto newModelRef = make_shared<ModelClass>();
//             // initialize new model with data from other model
//             newModelRef->set(otherModel->attributes());
//             // add it to our collection
//             add(newModel);
//         }
//     }




//
// string parseModelJsonValue(Json::Value &value);
//
// void registerSyncCallbacks(Collection<ModelClass> &otherCollection, bool _register = true){
//     if(_register){
//         ofAddListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
//         ofAddListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
//         ofAddListener(otherCollection.modelRemovedEvent, this, &Collection<ModelClass>::onSyncSourceModelRemoved);
//         ofAddListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
//     } else {
//         ofRemoveListener(otherCollection.modelAddedEvent, this, &Collection<ModelClass>::onSyncSourceModelAdded);
//         ofRemoveListener(otherCollection.modelChangedEvent, this, &Collection<ModelClass>::onSyncSourceModelChanged);
//         ofRemoveListener(otherCollection.modelRemovedEvent, this, &Collection<ModelClass>::onSyncSourceModelRemoved);
//         ofRemoveListener(otherCollection.collectionDestroyingEvent, this, &Collection<ModelClass>::onSyncSourceDestroying);
//     }
// }
