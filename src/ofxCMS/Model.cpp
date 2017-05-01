//
//  CMSModel.cpp
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#include "Model.h"
#include "ofxJSONElement.h"

using namespace ofxCMS;

const int Model::INVALID_CID = -1;

Model::Model() : mId(""), mCid(INVALID_CID){
}

Model* Model::set(const string &attr, const string &value, bool notify){
    string old_value = _attributes[attr];

    _attributes[attr] = value;
    onSetAttribute(attr, value);

    if(notify && old_value != value){
        static AttrChangeArgs args;
        args.model = this;
        args.attr = attr;
        args.value = value;
        onAttributeChanged(attr, value, old_value);

        if(notify){
            changeEvent.notifyListeners(*this);
            attributeChangedEvent.notifyListeners(args);
        }
    }

    // returning `this` allows the caller to link operations, like so:
    // model.set('name', 'Johnny')->set('surname', 'Blaze')->set('age', '44');
    return this;
}


Model* Model::set(map<string, string> &attrs, bool notify){
    for(map<string, string>::iterator it=attrs.begin(); it != attrs.end(); it++){
        this->set(it->first, it->second, notify);
    }

	return this;
}

string Model::get(const string &attr, string _default) const {
    return (_attributes.find(attr) == _attributes.end()) ? _default : _attributes.at(attr);
}

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
        if(json[i].isObject() && !json[i]["$oid"].isNull() && json[i]["$oid"].isString()){
            ids.push_back(json[i]["$oid"].asString());
            // simple string id
        } else if(json[i].isString()){
            ids.push_back(json[i].asString());
        } else if(json[i].isInt()){
            ids.push_back(ofToString(json[i].asInt()));
        } else if(json[i].isDouble()){
            ids.push_back(ofToString(json[i].asDouble()));
        // unknown/invalid value
        } else {
            ofLogWarning() << "Invalid value in json array: " << ((ofxJSONElement)json[i]).getRawString(false);
        }
    }

    return ids;
}
