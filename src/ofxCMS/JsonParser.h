#pragma once

#include "BaseCollection.h"
#ifdef OFXCMS_JSON
    #include "ofxJSONElement.h"
#endif

namespace ofxCMS{

    template<class ModelClass>
    class JsonParser {
    public:
        JsonParser() : collection(NULL), doCreate(true), doUpdate(true), doRemove(true){}
        // ~JsonParser(){ destroy(); }

        void setup(BaseCollection<ModelClass> *collection, const string& filename = "");
        // void destroy();

        bool load();
        bool loadRaw(const string& source){ return parse(source); }

    private:

        bool parse(const string &jsonText);
        bool parseModelJson(shared_ptr<ModelClass> modelRef, const string &jsonText);

#ifdef OFXCMS_JSON
        bool parseWithKeys(const ofxJSONElement &json);
        string processJsonValue(Json::Value &value);
        string idFromElement(Json::Value& node);
#endif

    private:
        BaseCollection<ModelClass> *collection;
        string filename;
        bool doCreate, doUpdate, doRemove;
    };
}

template<class ModelClass>
void ofxCMS::JsonParser<ModelClass>::setup(BaseCollection<ModelClass> *collection, const string& filename){
    this->collection = collection;
    this->filename = filename;
}

// template<class ModelClass>
// void ofxCMS::JsonParser<ModelClass>::destroy(){}
// }

template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::load(){
    ofBuffer content;
    ofFile file(filename);

    // check if file exists
    if(!file.exists()){
        ofLogWarning() << "file does not exist: " << filename;
        return false;
    }

    // read file content
    content = file.readToBuffer();

    // parse content
    return parse(content.getText());
}

template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parse(const string &jsonText){
#ifndef OFXCMS_JSON
    ofLogWarning() << "ofxCMS json not supported; enable OFXCMS_JSON preprocessor flag";
    return false;
#else
    ofxJSONElement json;

    // try to parse json, abort if it fails
    if(!json.parse(jsonText)){
        ofLogWarning() << "Couldn't parse JSON:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return false;
    }

    // make sure we've got an array, we're a collection after all
    if(!json.isArray()){
        // ofLogWarning() << "JSON is not an array:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return parseWithKeys(json);
    }

    if(doRemove){
        collection->each([&](shared_ptr<ModelClass> modelRef){
            string id = modelRef->getId();

            // assume we'll have to remove
            bool remove_this_model = true;

            // loop over all items in the new json to see if there's a record with the same id
            for(int j=0; j<json.size(); j++){
                // if there's a record with this id, we don't have to do anything
                if(idFromElement(json[j]) == id){
                    remove_this_model = false;
                    return;
                }
            }

            // if remove_model is still true, this means that no records with a matching id were found,
            // meaning this in-memory record was removed from the collection and we should drop it as well
            if(remove_this_model)
                collection->removeByCid(modelRef->cid());
        });
    }

    for(int i = 0; i < json.size(); i++) {
        string id = idFromElement(json[i]);
        auto existingRef = id == "" ? nullptr : collection->findById(id);

        // found existing model with same id? update it by setting its json attribute
        if(existingRef){
            if(doUpdate){
                // let the Model attribute changed callbacks deal with further parsing
                parseModelJson(existingRef, ((ofxJSONElement)json[i]).getRawString(false));
            }

            continue;
        }

        if(doCreate){
            //  not existing model found? Add a new one
            auto newRef = make_shared<ModelClass>();
            parseModelJson(newRef, ((ofxJSONElement)json[i]).getRawString(false));
            collection->add(newRef);
        }
    }

    return true;
#endif
}

#ifdef OFXCMS_JSON
template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parseWithKeys(const ofxJSONElement &json){

    // // make sure we've got an array, we're a collection after all
    // if(json.isArray()){
    //     ofLogWarning() << "JSON is an array:\n--JSON START --\n" << jsonText << "\n--JSON END --";
    //     return false;
    // }

    if(doRemove){
        collection->each([&](shared_ptr<ModelClass> modelRef){
            string id = modelRef->getId();

            // assume we'll have to remove
            bool remove_this_model = true;

            // loop over all items in the new json to see if there's a record with the same id
            vector<string> attrs = json.getMemberNames();
            for(auto& attr : attrs){
                // if there's a record with this id, we don't have to do anything
                if(attr == id){
                    remove_this_model = false;
                    return;
                }
            }

            // if remove_model is still true, this means that no records with a matching id were found,
            // meaning this in-memory record was removed from the collection and we should drop it as well
            if(remove_this_model)
                collection->removeByCid(modelRef->cid());
        });
    }

    // loop over all items in the new json to see if there's a record with the same id
    vector<string> attrs = json.getMemberNames();
    for(auto& id : attrs){
        auto existingRef = id == "" ? nullptr : collection->findById(id);

        // found existing model with same id? update it by setting its json attribute
        if(existingRef){
            if(doUpdate){
                // let the Model attribute changed callbacks deal with further parsing
                parseModelJson(existingRef, ((ofxJSONElement)json[id]).getRawString(false));
            }

            continue;
        }

        if(doCreate){
            //  not existing model found? Add a new one
            auto newRef = make_shared<ModelClass>();
            parseModelJson(newRef, ((ofxJSONElement)json[id]).getRawString(false));
            string attr_name = "id";
            while(newRef->has(attr_name))
                attr_name = "_"+attr_name;
            newRef->set(attr_name, id);
            collection->add(newRef);
        }
    }

    return true;
}
#endif

template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parseModelJson(shared_ptr<ModelClass> modelRef, const string &jsonText){
#ifndef OFXCMS_JSON
    ofLogWarning() << "ofxCMS json not supported; enable OFXCMS_JSON preprocessor flag";
    return false;
#else
    ofxJSONElement doc;

    if(!doc.parse(jsonText)){
        ofLogWarning() << "couldn't parse json:\n-- JSON start --\n" << jsonText << "\n-- JSON end --";
        return false;
    }

    vector<string> attrs = doc.getMemberNames();
    for(int i=0; i<attrs.size(); i++){
        modelRef->set(attrs[i], processJsonValue(doc[attrs[i]]));
    }

    return true;
#endif
}

#ifdef OFXCMS_JSON
template<class ModelClass>
string ofxCMS::JsonParser<ModelClass>::processJsonValue(Json::Value &value){
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

    //    while(unsigned i = val.find("\\\"") != string::npos)
    //        val.erase(i, 1);

    return val;
}

template<class ModelClass>
string ofxCMS::JsonParser<ModelClass>::idFromElement(Json::Value& node){
    return node["id"].isNull() ? "" : node["id"].asString();
}
#endif
