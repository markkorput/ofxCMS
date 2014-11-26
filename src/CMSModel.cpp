//
//  CMSModel.cpp
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#include "CMSModel.h"
#include "ofxJSONElement.h"

#define INVALID_CID (-1)

using namespace CMS;

int Model::mCidCounter = 0;

Model::Model(){
    // TODO: use a more globally unique timestamp-based Cid format?
    mCid = "c"+ofToString(mCidCounter);
    mCidCounter++;
}

Model::~Model(){
    ofNotifyEvent(beforeDestroyEvent, *this, this);
}

Model* Model::set(string attr, string value, bool notify){
    string old_value = _attributes[attr];

    _attributes[attr] = value;
    onSetAttribute(attr, value);

    if(old_value != value){
        static AttrChangeArgs args;
        args.attr = attr;
        args.value = value;
        ofNotifyEvent(attributeChangedEvent, args, this);
    }

    // returning `this` allows the caller to link operations, like so:
    // model.set('name', 'Johnny')->set('surname', 'Blaze')->set('age', '44');
    return this;
}


Model* Model::set(map<string, string> &attrs){
    for(map<string, string>::iterator it=attrs.begin(); it != attrs.end(); it++){
        this->set(it->first, it->second);
    }
}

string Model::get(string attr, string _default){
    return (_attributes.find(attr) == _attributes.end()) ? _default : _attributes[attr];
}

string Model::cid(){
    return mCid;
}

string Model::id(){
    // look for an "id" attribute, if that's not present,
    // look for an "_id" attribute (mongoDB style), if that's not present,
    // grab the cid()
    return get("id", get("_id", cid()));
}

// this was causing SIGABRT exceptions...
//void Model::destroy(bool notify){
//    if(notify) ofNotifyEvent(beforeDestroyEvent, *this, this);
//    delete this;
//}

// Convenience method with built-in support for MongoDB-style id format
vector<string> Model::jsonArrayToIdsVector(string jsonText){
    return jsonArrayToStringVector(jsonText);
}

// Convenience method with built-in support for MongoDB-style id format
vector<string> Model::jsonArrayToStringVector(string jsonText){
    vector<string> ids;
    ofxJSONElement json;

    if(!json.parse(jsonText)){
        ofLogWarning() << "Couldn't parse json: " << jsonText;
        return ids;
    }
    
    if(!json.isArray()){
        ofLogWarning() << "ObjectModel's personas attribute is not an array: " << jsonText;
        return ids;
    }
    
    // loop over each value in json array and add it to our ids vector
    for(int i=0; i<json.size(); i++){
        // mongoDB-style id
        if(json[i].isObject() && json[i]["$oid"] != NULL && json[i]["$oid"].isString()){
            ids.push_back(json[i]["$oid"].asString());
            // simple string id
        } else if(json[i].isString()){
            ids.push_back(json[i].asString());
            // invalid value
        } else {
            ofLogWarning() << "Invalid value in json array: " << ((ofxJSONElement)json[i]).getRawString(false);
        }
    }

    return ids;
}
