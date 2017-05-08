#pragma once

#include "ManagerBase.h"
#include "ofxJSONElement.h"

namespace ofxCMS{

    template<class CollectionClass>
    class JsonParserManager {
    public:
        JsonParserManager() : manager(NULL), doCreate(true), doUpdate(true), doRemove(false){}

        void setup(ManagerBase<CollectionClass> *collection, const string& filename);
        bool load();

    private:

        bool parse(const string &jsonText);
        bool parseWithKeys(const ofxJSONElement &json);

    private:
        ManagerBase<CollectionClass> *manager;
        string filename;
        bool doCreate, doUpdate, doRemove;
    };
}

template<class CollectionClass>
void ofxCMS::JsonParserManager<CollectionClass>::setup(ManagerBase<CollectionClass> *manager, const string& filename){
    this->manager = manager;
    this->filename = filename;
}

template<class CollectionClass>
bool ofxCMS::JsonParserManager<CollectionClass>::load(){
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

template<class CollectionClass>
bool ofxCMS::JsonParserManager<CollectionClass>::parse(const string &jsonText){
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

    ofLogWarning() << "Manager JSON is an array; this is not supported (yet);\n--JSON START --\n" << jsonText << "\n--JSON END --";
    return false;
}

template<class CollectionClass>
bool ofxCMS::JsonParserManager<CollectionClass>::parseWithKeys(const ofxJSONElement &json){
    if(!manager){
        ofLogWarning() << "not manager specified";
        return false;
    }

    if(doRemove){
        // for each collection currently in the manager, see if there's ANY data, if not; remove that collection
        manager->each([&](shared_ptr<CollectionClass> collectionRef, const string& name){
            // loop over all items in the new json to see if there's a record with the same id
            vector<string> attrs = json.getMemberNames();
            for(auto& attr : attrs){
                // if we find data for this collection, don't remove it
                if(attr == name)
                    return;
            }

            // didn't find data for this collection; remove it
            manager->remove(name);
        });
    }

    // loop over all items in the new json to see if there's a record with the same id
    vector<string> attrs = json.getMemberNames();
    for(auto& name : attrs){
        auto existingRef = name == "" ? nullptr : manager->get(name, false);

        // found existing model with same id? update it by setting its json attribute
        if(existingRef){
            if(doUpdate){
                existingRef->loadJson(((ofxJSONElement)json[name]).getRawString(false));
            }

            continue;
        }

        if(doCreate){
            existingRef = manager->get(name, true); // now do create new instance
            existingRef->loadJson(((ofxJSONElement)json[name]).getRawString(false));
        }
    }

    return true;
}
