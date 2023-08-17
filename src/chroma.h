#ifndef CHROMA_H
#define CHROMA_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <chrono>

#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

#include "chromatic.h"
#include "disco.h"

class ChromaRuntimeException;
class ChromaObject;
class ChromaData;
class ChromaFunction;
class ChromaEnvironment;
class ChromaController;
class DiscoMaster;

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
    STRING_TYPE, NUMBER_TYPE, OBJECT_TYPE, LIST_TYPE, NULL_TYPE
};

struct ChromaState {
    public:
        int pixel_length;
        float delta_time;
        float time;
        float get_time_diff(float prev) const { 
            return time - prev;
        }
};

class ChromaObject {
    private:
        std::string obj_typename;
    public:
        ChromaObject(const std::string& obj_typename) : obj_typename(obj_typename) { }
        virtual ~ChromaObject() {}
        const std::string& get_typename() const { return this->obj_typename; }
        virtual std::shared_ptr<ChromaObject> clone() { return std::make_shared<ChromaObject>(*this); }
};

class ChromaEffect : public ChromaObject {
    public:
        ChromaEffect() : ChromaObject("Effect") { }
        ChromaEffect(const std::string& sub_typename) : ChromaObject(sub_typename + "Effect") { }
        virtual ~ChromaEffect() { }
        virtual void tick(const ChromaState& state) { }
        virtual vec4 draw(float index, const ChromaState& state) const = 0;
};

class ChromaData {
    private:
        boost::variant <float, std::string, std::shared_ptr<ChromaObject>, std::vector<ChromaData>> data;
        ChromaType type;
    public:
        ChromaData(int int_val) : data(static_cast<float>(int_val)), type(NUMBER_TYPE) {}
        ChromaData(float float_val) :  data(float_val), type(NUMBER_TYPE) {}
        ChromaData(const std::string& string_val) : data(string_val), type(STRING_TYPE) {}
        ChromaData(const std::shared_ptr<ChromaObject>& obj_val) : data(obj_val), type(OBJECT_TYPE) { }
        ChromaData(ChromaObject* obj_val) : data(std::shared_ptr<ChromaObject>(obj_val)), type(OBJECT_TYPE) {}
        ChromaData(const std::vector<ChromaData>& list_val) : data(list_val), type(LIST_TYPE) {}
        ChromaData() : data(std::shared_ptr<ChromaObject>(nullptr)), type(NULL_TYPE) {}
        
        const ChromaType& get_type() const { return this->type; }
        
        float get_float() const { return boost::get<float>(this->data); }
        int get_int() const { return static_cast<int>(boost::get<float>(this->data)); }
        std::string get_string() const { return boost::get<std::string>(this->data); }
        const std::shared_ptr<ChromaObject>& get_obj() const { return boost::get<std::shared_ptr<ChromaObject>>(this->data); }
        template <class T>
        const std::shared_ptr<T> get_obj() const { return std::dynamic_pointer_cast<T>(boost::get<std::shared_ptr<ChromaObject>>(this->data)); }
        const std::shared_ptr<ChromaEffect> get_effect() const { return this->get_obj<ChromaEffect>(); }
        const std::vector<ChromaData>& get_list() const { return boost::get<std::vector<ChromaData>>(this->data); }
};

class ChromaFunction {
    std::string name;
    public:
        ChromaFunction(const std::string& name) : name(name) {}
        virtual ~ChromaFunction() { }
        std::string get_name() const { return name; }
        virtual ChromaData call(const std::vector<ChromaData>& args, ChromaEnvironment& env) = 0;
};

struct ChromaEnvironment {
    ChromaData ret_val;
    std::unordered_map<std::string, std::shared_ptr<ChromaFunction>> functions;
    std::unordered_map<std::string, ChromaData> variables;
    ChromaController* controller;
};

class ChromaController {
    private:
        std::string component_id = std::string("chroma.local");
        std::vector<std::shared_ptr<ChromaEffect>> layers = std::vector<std::shared_ptr<ChromaEffect>>({nullptr});
        size_t current_layer = 0;
        bool running = false;
    public:
        // TODO: return some kind of status?
        void set_effect(const std::shared_ptr<ChromaEffect>& effect) {
            this->layers[this->current_layer] = effect;
        }
        void add_layer() {
            this->layers.push_back(nullptr);
        }
        void set_current_layer(size_t index) {
            this->current_layer = index;
        }
        size_t get_num_layers() {
            return this->layers.size();
        }
        size_t get_current_layer() {
            return this->current_layer;
        }
        std::string get_component_id() {
            return this->component_id;
        }
        void set_component_id(const std::string& id) {
            this->component_id = id;
        }
        void stop() {
            this->running = false;
        }
        bool is_running() {
            return this->running;
        }
        void run(int fps, size_t pixel_length, std::function<int(const std::vector<vec4>&)> callback);
        void run(int fps, size_t pixel_length, DiscoMaster& disco);
};

#endif