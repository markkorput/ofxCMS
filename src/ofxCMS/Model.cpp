//
//  CMSModel.cpp
//  ofxCMS
//
//  Created by Mark van de Korput on 16/09/14.
//
//

#include "Model.h"

#ifdef OFXCMS_JSON
    #include "ofxJSONElement.h"
#endif

using namespace ofxCMS;

Model* Model::set(const string &attr, const string &value, bool notify){
    if(isLocked()){
        ofLogVerbose() << "model locked; queueing .set operation for attribute: " << attr;
        modQueueRefs.push_back(make_shared<Mod>(attr, value, notify));
        return this;
    }

    string old_value = _attributes[attr];

    _attributes[attr] = value;
    onSetAttribute(attr, value);

    if(notify && old_value != value){
        AttrChangeArgs args;
        args.model = this;
        args.attr = attr;
        args.value = value;
        onAttributeChanged(attr, value, old_value);

        if(notify){
            changeEvent.notifyListeners(*this);
            attributeChangeEvent.notifyListeners(args);
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
    return has(attr) ? _attributes.at(attr) : _default;
}

bool Model::has(const string& attr) const {
    return (_attributes.find(attr) == _attributes.end()) ? false : true;
}

void Model::each(AttrIterateFunc func){
    lock([this, &func](){
        for(auto pair : this->_attributes){
            func(pair.first, pair.second);
        }
    });
}

void Model::copy(shared_ptr<Model> otherRef, bool also_ids){
    if(!otherRef){
        ofLogWarning() << "ofxCMS::Model.copy got nullptr";
        return;
    }

    copy(*otherRef.get(), also_ids);
}

void Model::copy(Model& other, bool also_ids){
    other.each([this, also_ids](const string& key, const string& value){
        if((key != "id" && key != "_id") || also_ids)
            this->set(key, value);
    });
}

void Model::lock(LockFunctor func){
    lockCount++;
    func();
    lockCount--;

    // still (recursively) locked? skip processing of opereations queue
    if(isLocked())
        return;

    // after we're done iterating, we should process any items
    // accumulated in the vector modificaton queue
    for(auto modRef : modQueueRefs){
        set(modRef->attr, modRef->value, modRef->notify);
        // TODO; add support for removing attributes?
    }

    modQueueRefs.clear();
}

#ifdef OFXCMS_JSON
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
#endif
