#ifndef CHROMA_H
#define CHROMA_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

#include <iostream>

class ChromaRuntimeException;
class ChromaObject;
class ChromaData;
class ChromaFunction;
class ChromaEnvironment;

class ChromaRuntimeException : public std::exception {
    private:
        const std::string message;
    public:
        ChromaRuntimeException(const char * msg) : message(msg) { }
        const char* what () {
            return this->message.c_str();
        }
};

enum ChromaType {
    String, Number, Effect, Object, List, Null
};

class ChromaObject {
    private:
        std::string obj_typename;
    public:
        virtual ~ChromaObject() {}
        std::string get_typename() const { return this->obj_typename; }
};

class ChromaData {
    private:
        boost::variant <float, std::string, std::shared_ptr<ChromaObject>, std::vector<ChromaData>> data;
        ChromaType type;
    public:
        ChromaData(int int_val) : data(static_cast<float>(int_val)), type(Number) {}
        ChromaData(float float_val) :  data(float_val), type(Number) {}
        ChromaData(const std::string& string_val) : data(string_val), type(String) {}
        ChromaData(const std::shared_ptr<ChromaObject>& obj_val) : data(obj_val), type(Object) { }
        ChromaData(ChromaObject* obj_val) : data(std::shared_ptr<ChromaObject>(obj_val)), type(Object) {}
        ChromaData(const std::vector<ChromaData>& list_val) : data(list_val), type(List) {}
        ChromaData() : data(std::shared_ptr<ChromaObject>(nullptr)), type(Null) {}
        
        const ChromaType& get_type() const { return this->type; }
        
        float get_float() const { return boost::get<float>(this->data); }
        std::string get_string() const { return boost::get<std::string>(this->data); }
        const std::shared_ptr<ChromaObject>& get_obj() const { return boost::get<std::shared_ptr<ChromaObject>>(this->data); }
        const std::vector<ChromaData>& get_list() const { return boost::get<std::vector<ChromaData>>(this->data); }
};

class ChromaFunction {
    std::string name;
    public:
        ChromaFunction(const std::string& name) : name(name) {}
        std::string get_name() const { return name; }
        virtual ChromaData call(const std::vector<ChromaData>& args, const ChromaEnvironment& env) = 0;
};

struct ChromaEnvironment {
    ChromaData ret_val;
    std::unordered_map<std::string, std::shared_ptr<ChromaFunction>> functions;
    std::unordered_map<std::string, ChromaData> variables;

    // ChromaEnvironment() {}
    // ChromaEnvironment(const ChromaEnvironment& other) : ret_val(ret_val), variables(variables) {

    // }
};

#endif