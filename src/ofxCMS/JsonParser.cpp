// template <class ModelClass>
// bool ofxCMS::Collection<ModelClass>::parse(const string &jsonText, bool doRemove, bool doUpdate, bool doCreate){
//     ofxJSONElement json;
//
//     // try to parse json, abort if it fails
//     if(!json.parse(jsonText)){
//         ofLogWarning() << "Couldn't parse JSON:\n--JSON START --\n" << jsonText << "\n--JSON END --";
//         return false;
//     }
//
//     // make sure we've got an array, we're a collection after all
//     if(!json.isArray()){
//         ofLogWarning() << "JSON not an array:\n--JSON START --\n" << jsonText << "\n--JSON END --";
//         return false;
//     }
//
//     if(doRemove){
//         // loop over all models that were already in the collection,
//         // remove any model for which we can't find any record in the new json
//         // IMPORTANT! Gotta start with the highest indexes first, because otherwise indexes
//         // of higher-up models get messed up when removing models earlier in the list
//         for(int i=count()-1; i>=0; i--){
//             // assume we'll have to remove the model as long as we haven't found a matching record in the json
//             bool remove_model = true;
//
//             // get the current model's id to match on
//             string id = at(i)->id();
//
//             // loop over all items in the new json to see if there's a record with the same id
//             for(int j=0; j<json.size(); j++){
//                 // if there's a record with this id, we don't have to do anything
//                 if(json[j]["_id"]["$oid"] == id)
//                     remove_model = false;
//             }
//
//             // if remove_model is still true, this means that no records with a matching id were found,
//             // meaning this in-memory record was removed from the collection and we should drop it as well
//             if(remove_model){
//                 remove(i);
//             }
//         }
//     }
//
//     for(int i = 0; i < json.size(); i++) {
//         auto existingRef = json[i]["_id"]["$oid"].isNull() ? NULL : findById(json[i]["_id"]["$oid"].asString());
//
//         // found existing model with same id? update it by setting its json attribute
//         if(existingRef){
//             if(doUpdate){
//                 // let the Model attribute changed callbacks deal with further parsing
//                 parseModelJson(existingRef, ((ofxJSONElement)json[i]).getRawString(false));
//             }
//
//             continue;
//         }
//
//         if(doCreate){
//             // do an early limit check, to avoid unnecessary parsing
//             if(limitReached() && !bFIFO){
//                 ofLogNotice(LOGNAME) << "parsing: model skipped because limit reached (NO FIFO)";
//                 continue;
//             }
//
//             //  not existing model found? Add a new one
//             auto newRef = make_shared<ModelClass>();
//
//             parseModelJson(newRef, ((ofxJSONElement)json[i]).getRawString(false));
//             // if we couldn't add this model to the collection
//             // destroy the model, otherwise it's just hanging out in memory
//             add(newRef);
//         }
//     }
//
//     ofLogVerbose(LOGNAME) << "parsing: finished, number of models in collection: " << count();
//     ofNotifyEvent(collectionInitializedEvent, this);
//     return true;
// }
//
// // for convenience
// template <class ModelClass>
// bool ofxCMS::Collection<ModelClass>::parse(const ofxJSONElement & node, bool doRemove, bool doUpdate, bool doCreate){
//     if(node.type() == Json::nullValue) return false;
//
//     // Can't figure out how to use this kinda object, so for now; let the text-based parse method deal with it
//     // (meaning we'll convert back to text, and parse that to json again... yea...)
//     if(node.type() == Json::stringValue) return parse(node.asString(), doRemove, doUpdate, doCreate);
//     return parse(node.getRawString(), doRemove, doUpdate, doCreate);
// }
//
// template <class ModelClass>
// void ofxCMS::Collection<ModelClass>::parseModelJson(shared_ptr<ModelClass>, const string &jsonText){
//     ofxJSONElement doc;
//
//     if(!doc.parse(jsonText)){
//         ofLogWarning() << "ofxCMS::Collection::parseModelJson() - couldn't parse json:\n-- JSON start --\n" << jsonText << "\n-- JSON end --";
//         return;
//     }
//
//     vector<string> attrs = doc.getMemberNames();
//     for(int i=0; i<attrs.size(); i++){
//         model->set(attrs[i], parseModelJsonValue(doc[attrs[i]]));
//     }
// }
//
// template <class ModelClass>
// string ofxCMS::Collection<ModelClass>::parseModelJsonValue(Json::Value &value){
//     //    return value.asString();
//     if(value.isObject() && value.isMember("$oid")) return value["$oid"].asString();
//     if(value.isObject() && value.isMember("$date")) return ofToString(value["$date"]);
//     if(value.isObject()) return ((ofxJSONElement)value).getRawString(false);
//     // here's a real clumsy way of removing leading and trailing white-space and double-quotes;
//     string val = ofToString(value);
//     // trim left
//     val.erase(0, val.find_first_not_of(" \n\r\t"));
//     // trim right
//     val.erase(val.find_last_not_of(" \n\r\t")+1);
//     // trim leading quote
//     if(val.find('"') == 0) val.erase(0, 1);
//     // trim trailing quote
//     if(val.rfind('"') == val.length()-1) val.erase(val.length()-1);
//
// //        while(unsigned i = val.find("\\\"") != string::npos)
// //            val.erase(i, 1);
//
//     return val;
// }
