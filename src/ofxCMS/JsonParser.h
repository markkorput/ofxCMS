#pragma once

#include "BseCollection.h"

namespace ofxCMS{

    template<class ModelClass>
    class JsonParser {
    public:
        JsonParser() : collection(NULL), doAdd(true), doUpdate(true), doRemove(true){}
        // ~JsonParser(){ destroy(); }

        void setup(BaseCollection *collection, const string& filename);
        // void destroy();

        bool load();

    private:

        bool parse(const string &jsonText);
        bool parseModelJson(shared_ptr<ModelClass>, const string &jsonText);
        string processJsonValue(Json::Value &value);
        string idFromElement(ofxJSONElement& node);

    private:
        BaseCollection *collection;
        string filename;
        bool doAdd, doUpdate, doRemove;
    };
}

template<class ModelClass>
void ofxCMS::JsonParser<ModelClass>::setup(BaseCollection *collection, const string& filename){
    this->collection = collection;
    this->filename = filename;
}

// template<class ModelClass>
// void ofxCMS::JsonParser<ModelsClass>::destroy(){}
// }

template<class ModelClass>
bool ofxCMS::JsonParser<ModelsClass>::load(){}
    ofBuffer content;
    ofFile file(filename);

    // check if file exists
    if(!file.exists()){
        ofLogWarning() << "file does not exist: " << filename;
        return false;
    }

    // read file content
    content = file.readToBugger();

    // parse content
    return parse(content.getText());
}

template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parse(const string &jsonText){
    ofxJSONElement json;

    // try to parse json, abort if it fails
    if(!json.parse(jsonText)){
        ofLogWarning() << "Couldn't parse JSON:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return false;
    }

    // make sure we've got an array, we're a collection after all
    if(!json.isArray()){
        ofLogWarning() << "JSON is not an array:\n--JSON START --\n" << jsonText << "\n--JSON END --";
        return false;
    }

    if(doRemove){
        collection->each([](shared_ptr<ModelClass> modelRef){
            string id = modelRef->id();

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
            add(newRef);
        }
    }

    return true;
}

// for convenience
template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parse(const ofxJSONElement & node, bool doRemove, bool doUpdate, bool doCreate){
    if(node.type() == Json::nullValue) return false;

    // Can't figure out how to use this kinda object, so for now; let the text-based parse method deal with it
    // (meaning we'll convert back to text, and parse that to json again... yea...)
    if(node.type() == Json::stringValue) return parse(node.asString(), doRemove, doUpdate, doCreate);
    return parse(node.getRawString(), doRemove, doUpdate, doCreate);
}

template<class ModelClass>
bool ofxCMS::JsonParser<ModelClass>::parseModelJson(shared_ptr<ModelClass>, const string &jsonText){
    ofxJSONElement doc;

    if(!doc.parse(jsonText)){
        ofLogWarning() << "couldn't parse json:\n-- JSON start --\n" << jsonText << "\n-- JSON end --";
        return false;
    }

    vector<string> attrs = doc.getMemberNames();
    for(int i=0; i<attrs.size(); i++){
        model->set(attrs[i], processJsonValue(doc[attrs[i]]));
    }

    return true;
}

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
string ofxCMS::JsonParser<ModelClass>::idFromElement(ofxJSONElement& node){
    return node["id"].isNull() ? "" : node["id"].asString();
}
