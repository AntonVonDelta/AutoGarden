#include "FixedString.h"

class JSON{
    //String data="";
    int mode=0; //0- object, 1-array
    FixedString<4800> data;
    
    // char string_buffer[5000];

    // void clear(){string_buffer[0]=0;}
    // char* concat(const char* str){
    //     strcat(string_buffer,str); 
    //     return string_buffer;
    // }
    // char* concat(const char* str1,const char* str2,const char* str3,const char* str4,const char* str5){
    //     strcat(string_buffer,str1); 
    //     strcat(string_buffer,str2); 
    //     strcat(string_buffer,str3); 
    //     strcat(string_buffer,str4); 
    //     strcat(string_buffer,str5); 
    //     return string_buffer;
    // }


    public:
        JSON(){
            data.clear();
        }

        void set(const char* name,const char* value){
            //concat("\"",name,"\":\"",value,"\",");
            //data+=String("\"")+String(name)+String("\":\"")+String(value)+String("\",");
            data.appendFormat("\"%s\":\"%s\",", name, value);
            //data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":\"")+FixedString<20>(value)+FixedString<4>("\",");
        }
        void set(const char* name,String value){
            //concat("\"",name,"\":\"",value.c_str(),"\",");
            data.appendFormat("\"%s\":\"%s\",", name, value.c_str());
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":\"")+FixedString<20>(value)+FixedString<4>("\",");
        }
        void set(const char* name,bool value){
            //concat("\"",name,"\":\"",value?"true":"false","\",");
            data.appendFormat("\"%s\":%s,", name, (value?"true":"false"));
            // FixedString<6> temp(value?"true":"false");
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":")+temp+FixedString<4>(",");
        }
        void set(const char* name,int value){
            //char int_buffer[100];
            //concat("\"",name,"\":\"",itoa(value,int_buffer,10),"\",");
            data.appendFormat("\"%s\":\"%i\",", name, value);
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":")+FixedString<20>(value)+FixedString<4>(",");
        }
        void set(const char* name,unsigned int value){
            //char int_buffer[100];
            //concat("\"",name,"\":\"",utoa(value,int_buffer,10),"\",");
            data.appendFormat("\"%s\":\"%u\",", name, value);
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":")+FixedString<20>(value)+FixedString<4>(",");
        }
        void set(const char* name,double value){
            data.appendFormat("\"%s\":\"%f\",", name, value);
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":")+FixedString<20>(value)+FixedString<4>(",");
        }

        void createArray(const char* name){
            mode=1;
            //concat("\"",name,"\":[");
            data.appendFormat("\"%s\":[", name);
            // data+=FixedString<4>("\"")+FixedString<20>(name)+FixedString<4>("\":[");
        }
        void closeArray(){
            mode=0;
            data.trimEnd(',');
            data+="],";
        }

        void createObject(){
            data.append('{');
        }
        void closeObject(){
            data.trimEnd(',');
            data.append('}');
            data.append(',');
        }

        void finish(){
            data.trimEnd(',');
        }

        // void addObject(JSON& other){
        //     if(mode==1) {
        //         data+=other.c_str();
        //         data.append(',');
        //     }
        // }
        // void addArrayElement(const char* value){
        //     if(mode==1) {
        //         data+=value;
        //         data.append(',');
        //     }
        // }

        
        String get(){
            return String(data.c_str());
        }

        const char* c_str(){
            return data.c_str();
        }
};