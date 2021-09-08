
class DynamicDebug{

private:
    static int debug_message_len;
    static char* debug_messages;

public:
    static void begin(){
        debug_messages=new char[debug_message_len];
        memset(debug_messages,0,debug_message_len);
    }
    static void print(const char* str){
        int len=strlen(str)+1;
        int shift=0;
        int ocuppied_space=strlen(debug_messages);
        int free_space=debug_message_len-1-ocuppied_space;
        
        if(len==0) return;
        if(len>=debug_message_len){
            len=debug_message_len-1;
        }
        
        if(len>free_space)shift=len-free_space;
        
        for(int i=0;i<=debug_message_len-2-shift;i++){
            debug_messages[i]=debug_messages[i+shift];
        }

        debug_messages[ocuppied_space-shift]=0;
        debug_messages[strlen(debug_messages)+len-1]='\n';
        memcpy(debug_messages+strlen(debug_messages),str,len-1);
        
        debug_messages[debug_message_len-1]=0;

        Serial.println(str);
    }


    static void print(String str){
        print(str.c_str());
    }


    static void print(uint64_t number){
        uint64_t base=10;
        size_t n = 0;
        char buf[64];
        uint8_t i = 0;
        uint8_t total;

        if (number == 0)
        {
            print("0");
            return;
        }
        while (number > 0){
            uint64_t q = number/base;
            buf[i++] = '0'+(number - q*base);

            number = q;
        }

        total=i;
        for(i=0;i<total/2;i++){
            char temp=buf[i];
            buf[i]=buf[total-i-1];
            buf[total-i-1]=temp;
        }

        buf[total]=0;
        print(buf);
    }
    static void print(IPAddress ip){
        print(ip.toString());
    }


    static void print(const char* val1,const char* val2){
        int len1=strlen(val1);
        int len2=strlen(val2);
        char buff[1 + len1+len2];

        memcpy(buff,val1,len1);
        memcpy(buff+len1,val2,len2);
        buff[len1+len2]=0;

        print(buff);
    }

    static void print(const char* val1,time_t val2){
        print(val1,(unsigned int)val2);
    }

    static void print(const char* val1,unsigned int val2){
        int len1=strlen(val1);
        int len2=33;
        char buff[1 + len1+len2];

        memcpy(buff,val1,len1);
        utoa(val2,buff+len1,10);    // This actually is a fail safe. The null is already added by itoa
        buff[len1+len2]=0;

        print(buff);
    }
    static void print(const char* val1,int val2){
        int len1=strlen(val1);
        int len2=33;
        char buff[1 + len1+len2];

        memcpy(buff,val1,len1);
        itoa(val2,buff+len1,10);    // This actually is a fail safe. The null is already added by itoa
        buff[len1+len2]=0;

        print(buff);
    }
    static void print(const char* str1,bool val2){
        const char* str2=(val2?"true":"false");
        print(str1,str2);
    }
    static void print(const char* val1,IPAddress ip){
        print(val1,ip.toString().c_str());
    }
    static void print(const char* val1,esp_reset_reason_t reason){
        const char* txt="nothing";
        if(reason==ESP_RST_UNKNOWN) txt="ESP_RST_UNKNOWN";
        if(reason==ESP_RST_POWERON) txt="ESP_RST_POWERON";
        if(reason==ESP_RST_EXT) txt="ESP_RST_EXT";
        if(reason==ESP_RST_SW) txt="ESP_RST_SW";
        if(reason==ESP_RST_PANIC) txt="ESP_RST_PANIC";
        if(reason==ESP_RST_INT_WDT) txt="ESP_RST_INT_WDT";
        if(reason==ESP_RST_TASK_WDT) txt="ESP_RST_TASK_WDT";
        if(reason==ESP_RST_WDT) txt="ESP_RST_WDT";
        if(reason==ESP_RST_DEEPSLEEP) txt="ESP_RST_DEEPSLEEP";
        if(reason==ESP_RST_BROWNOUT) txt="ESP_RST_BROWNOUT";
        if(reason==ESP_RST_SDIO) txt="ESP_RST_SDIO";

        print(val1,txt);
    }


    static const char* c_str(){
        return debug_messages;
    }
};

int DynamicDebug::debug_message_len=2560;
char* DynamicDebug::debug_messages;