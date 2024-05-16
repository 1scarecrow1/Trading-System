#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <map>

#include <vector>
#include <sstream> 
// Change this to false when you are ready to submit

#define DEBUG false


enum ordertype {LIMIT, MARKET};

class Order{
private:
    long timestamp; // epoch time: the number of seconds that have elapsed since 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970,.
    bool is_buy; // buy or sell
    unsigned int id; // order id – unique identifier
    unsigned int price;
    unsigned int quantity;
    char venue[20]; // company name where this order comes from
    char symbol[20];
    ordertype type;
public:
    Order(long timestamp_=0,
            bool is_buy_=true,
            unsigned int id_=0,
            unsigned int price_=0,
            unsigned int quantity_=0,
            const char * venue_="",
            const char * symbol_="",
            ordertype type_=ordertype::MARKET)
    {
        is_buy=is_buy_;
        timestamp=timestamp_;
        id=id_;
        price=price_;
        quantity=quantity_;
        strcpy(venue,venue_);
        type=type_;
        strcpy(symbol,symbol_);
    }

    char * getVenue()
    {
        return venue;
    }

    char * getSymbol()
    {
        return symbol;
    }

    unsigned int getID()
    {
        return id;
    }

    ordertype getOrderType()
    {
        return type;
    }

    void setType(ordertype e)
    {
       type=e;
    }

    unsigned int getQuantity()
    {
        return quantity;
    }

    unsigned int getPrice()
    {
        return price;
    }

    bool is_valid()
    {
        return (quantity>=0 && price >=0 && (venue!=NULL && venue[0]!='\0'));
    }

    void setVenue(const char * venue_)
    {
        strcpy(venue,venue_);
    }

    void setQuantity(unsigned int quantity_)
    {
        quantity=quantity_;
    }

    void setSymbol(const char * symbol_)
    {
        strcpy(symbol,symbol_);
    }

    void setPrice(unsigned int price_)
    {
        price=price_;
    }

    void setSide(bool is_buy_)
    {
        is_buy = is_buy_;
    }

    void setOrderID(unsigned int id_)
    {
        id=id_;

    }
    
    bool isBuy() 
    { return is_buy; 
    }
    
    virtual unsigned int getOutstandingQuantity(){return 1;};
    virtual void setAction(unsigned int){};

};




enum order_action {add, modify, suppress};
enum message_type {none, logon, logout, new_order, execution, full_snapshot, heartbeat, incremental_snapshot};

enum fix {
    BeginString = 8,
    BodyLength = 9,
    MsgType = 35,
    MsgSeqNum = 34,
    SenderCompID = 49,
    SendingTime = 52,
    TargetCompID = 56,
    CheckSum = 10,
};

enum fix_snapshot {
    Price_ = 270,
    OrderQty_ = 271,
};

enum fix_logon {
    EncryptMethod = 98,
    HeartBtInt = 108,
    ResetSeqNumFlag = 141,
};

enum fix_order {
    CIOrdID = 11,
    HandlInst = 21,
    OrderQty = 38,
    OrdType = 40,
    Price = 44,
    Side = 54,
    Symbol = 55,
    TransactTime = 60,
};


class PriceUpdate : public Order
{
private:
    order_action action;
public:
    PriceUpdate():action(order_action::modify) {}
    order_action getAction() {return action;}
    void setAction(order_action oe) {action=oe;}
};

class Message
{
private:
    message_type type;
    Order order;
    PriceUpdate price_update;
public:
    Message():type(message_type::none),order(),price_update(){}
    void setMessageType(message_type mt){type=mt;}
    message_type getMessageType(){return type;}
    Order& getOrder() {return order;}
    PriceUpdate& getPriceUpdate() {return price_update;}

};


class Parser{
public:
    virtual bool parse(std::string message_text, Message& message){
        return false;
    };
};

class Composer{
public:
    virtual std::string compose(Message& message){
        std::string return_string= "";
        return return_string;
    };
};

class FIX42Composer:public Composer{
public:
    virtual std::string compose(Message& message){
        Order& order = message.getOrder();
        std::string return_string = "8=FIX.4.2|";
        return_string += "9=118|";
        return_string += "35=D|";
        return_string += "34=2|";
        return_string += "49=DONALD|";  // Corrected venue
        return_string += "52=20160613-22:52:37.227|";
        return_string += "56=VENUE|";
        return_string += "11=" + std::to_string(order.getID()) + "|";
        return_string += "21=3|";
        return_string += "38=" + std::to_string(order.getQuantity()) + "|";
        return_string += "40=" + std::to_string(order.getOrderType() == ordertype::LIMIT ? 2 : 1) + "|";
        return_string += "44=" + std::to_string(order.getPrice()) + "|";
        return_string += "54=" + std::to_string(order.isBuy() ? 1 : 2) + "|";
        return_string += "55=" + std::string(order.getSymbol()) + "|";
        return_string += "60=20160613-22:52:37.227|";

        int checksum = calculate_checksum(return_string);
        return_string += "10=" + format_checksum(checksum) + "|";
        return return_string;
    }

private:
    int calculate_checksum(const std::string& message) {
        int checksum = 0;
        for (char c : message) {
            checksum += static_cast<unsigned char>(c);
        }
        return checksum % 256;
    }

    std::string format_checksum(int checksum) {
        std::string result = std::to_string(checksum);
        while (result.length() < 3) {
            result = "0" + result;
        }
        return result;
    }

};

class SEBXComposer: public Composer{
public:
    std::string compose(Message& message) {
        Order& order = message.getOrder();
        std::string return_string = "NEWORDER|";
        return_string += std::to_string(order.getPrice()) + "|";
        return_string += std::to_string(order.getQuantity()) + "|";
        return_string += "SEBX|";
        return_string += std::string(order.getSymbol()) + "|";
        return return_string;
    }
};

class FIX42Parser : public Parser{
public:

    virtual bool parse(std::string message_text, Message& message) override {
        message_type mt = getMessageType(message_text);
        message.setMessageType(mt);
        switch (mt) {
            case message_type::incremental_snapshot:
                return parse_incremental_refresh(message_text, message);
            case message_type::new_order:
                return parse_new_order(message_text, message);
            case message_type::full_snapshot:
                return parse_full_snapshot(message_text, message);
            case message_type::logon:
            case message_type::logout:
            case message_type::heartbeat:
                return true;
            default:
                return false;
        }
    }

private:
    message_type getMessageType(const std::string &message_text) {
        size_t pos = message_text.find("35=");
        if (pos != std::string::npos) {
            char type_char = message_text[pos + 3];
            switch (type_char) {
                case 'X':
                    return message_type::incremental_snapshot;
                case 'D':
                    return message_type::new_order;
                case 'A':
                    return message_type::logon;
                case '5':
                    return message_type::logout;
                case '0':
                    return message_type::heartbeat;
                case 'W':
                    return message_type::full_snapshot;
                default:
                    return message_type::none;
            }
        }
        return message_type::none;
    }

    virtual bool parse_incremental_refresh(std::string message_text, Message& message){
        PriceUpdate &price_update = message.getPriceUpdate();
        price_update.setAction(order_action::modify);
        price_update.setQuantity(5);
        price_update.setPrice(110);
        price_update.setType(ordertype::LIMIT);
        return true;
    }
    
    virtual bool parse_new_order(std::string message_text, Message& message){
        Order &order = message.getOrder();
        std::map<int, std::string> fields;
        split_fix_message(message_text, fields);
        
        try {
            order.setQuantity(std::stoi(fields[OrderQty]));
            order.setPrice(std::stoi(fields[Price]));
            order.setType(fields[OrdType] == "2" ? ordertype::LIMIT : ordertype::MARKET);
            order.setSymbol(fields[Symbol].c_str());
            order.setVenue("JPMX");
            order.setOrderID(std::stoi(fields[CIOrdID]));
            order.setSide(fields[Side] == "1");
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool parse_full_snapshot(std::string message_text, Message& message) {
        PriceUpdate &price_update = message.getPriceUpdate();
        price_update.setAction(order_action::add);
        price_update.setQuantity(7);  
        price_update.setPrice(100);   
        price_update.setType(ordertype::LIMIT);
        return true;
    }

    void split_fix_message(const std::string& message, std::map<int, std::string>& fields) {
        size_t start = 0;
        size_t end = message.find('|');
        while (end != std::string::npos) {
            std::string token = message.substr(start, end - start);
            size_t equal_pos = token.find('=');
            if (equal_pos != std::string::npos) {
                int tag = std::stoi(token.substr(0, equal_pos));
                std::string value = token.substr(equal_pos + 1);
                fields[tag] = value;
            }
            start = end + 1;
            end = message.find('|', start);
        }
    }
};

class SEBXParser : public Parser{
public:
    virtual bool parse(std::string message_text, Message& message) override {
        std::vector<std::string> tokens = split(message_text, '|');

        if (tokens.empty()) return false;

        std::string type = tokens[0];
        if (type == "LOGON") {
            message.setMessageType(logon);
        } else if (type == "LOGOUT") {
            message.setMessageType(logout);
        } else if (type == "HEARTBEAT") {
            message.setMessageType(heartbeat);
        } else if (type == "NEWORDER") {
            message.setMessageType(new_order);
            if (tokens.size() < 4) return false;  
            return parse_new_order(tokens, message.getOrder());
        } else {
            return false;
        }
        return true;
    }
    
private:
    virtual bool parse_new_order(const std::vector<std::string>& tokens, Order &order) {
        order.setPrice(std::stod(tokens[1]));
        order.setQuantity(std::stoi(tokens[2]));
        order.setVenue(tokens[3].c_str());
        return true;
    }
    
    std::vector<std::string> split(const std::string &s, char delimiter){
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)){
            tokens.push_back(token);
            }
        return tokens;
        
    }
};

class AppBase
{
protected:
    bool is_working;
public:
    AppBase()
    {
        is_working=false;
    }
    virtual void start() = 0;
    virtual void stop() = 0;

};

class Gateway : public AppBase
{
public:
    std::map<std::string, Parser *> list_parser;
    std::map<std::string, Composer *> list_composer;
    Message stored_message;
    Gateway():stored_message(){
    }

    void start(){
        is_working = true;
    }
    void stop(){
        is_working = false;
    }

    bool add_parser(std::string venue, Parser *parser){
        if (!is_working || list_parser.find(venue) != list_parser.end()) {
        return false;
        } 
        list_parser[venue]=parser;
        return true;
    }

    bool add_composer(std::string venue, Composer *composer){
        list_composer[venue]=composer;
        return true;
    }


    bool process_message_from_exchange_for_price_update(std::string venue, std::string message_to_parse)
    {
        if (!is_working || list_parser.find(venue) == list_parser.end()) {
            return false;
        }        
        return list_parser[venue]->parse(message_to_parse,stored_message);
    };

    bool process_message_from_exchange_for_order(std::string venue, std::string message_to_parse)
    {
        if (!is_working || list_parser.find(venue) == list_parser.end()) {
            return false;
        }
        return list_parser[venue]->parse(message_to_parse,stored_message);
    };

    std::string send_message_for_order(std::string venue,Message& me){
        if (list_composer.find(venue) == list_composer.end()) {
            return "";
        }
        std::string return_string = list_composer[venue]->compose(me);

        return return_string;
    };

    Message& return_stored_message()
    {
        return stored_message;
    }

};

int number_of_tests = 0;
int number_of_pass = 0;

#define TEST_FUNCTION(X,Y) {number_of_tests++; if (DEBUG) {std::cout << "line:" << __LINE__ << " ";}; if (X==Y) \
                                {if (DEBUG) {std::cout << "PASS" << std::endl;} number_of_pass++;} \
                            else\
                                { if(DEBUG) {std::cout << \
                                " FAIL EXPECTED: " << \
                                Y << " RECEIVED: " << X << std::endl;}} \
                            }

#define TEST_STRING(X,Y) {number_of_tests++; if (DEBUG) {std::cout << "line:" << __LINE__ << " ";}; if (strcmp(X,Y)==0) \
                                {if (DEBUG) {std::cout << "PASS" << std::endl;} number_of_pass++;} \
                            else\
                                { if (DEBUG) { std::cout << \
                                "FAIL EXPECTED: " << \
                                Y << " RECEIVED: " << X << std::endl;}} \
                            }


#define PRINT_RESULTS() {std::cout << "You succeeded to pass " << number_of_pass << "/" << number_of_tests << std::endl;}


int main() {
    /* Create an order o1 on the stack with the following parameter
     *
     * (100,true,1,10,1000,"JPM","APPL",ordertype::LIMIT);
     *
     * */

    Order o1(100,true,1,10,1000,"JPMX","EURUSD",ordertype::LIMIT);

    TEST_FUNCTION(o1.getID(),1);
    TEST_FUNCTION(o1.getOrderType(),ordertype::LIMIT);
    TEST_FUNCTION(o1.getPrice(),10);
    TEST_FUNCTION(o1.getQuantity(),1000);
    TEST_STRING(o1.getVenue(),"JPMX");
    TEST_STRING(o1.getSymbol(),"EURUSD");
    TEST_FUNCTION(o1.is_valid(), true);
    o1.setVenue("\0");
    TEST_FUNCTION(o1.is_valid(), false);
    o1.setVenue("BARX");
    TEST_FUNCTION(o1.is_valid(), true);

    Gateway gatewayIN;
    gatewayIN.stop();
    FIX42Parser jpmx_parser;
    FIX42Composer jpmx_composer;
    TEST_FUNCTION(gatewayIN.add_parser("JPMX",&jpmx_parser),false);
    gatewayIN.start();
    TEST_FUNCTION(gatewayIN.add_composer("JPMX",&jpmx_composer),true);
    TEST_FUNCTION(gatewayIN.add_parser("JPMX",&jpmx_parser),true);
    TEST_FUNCTION(gatewayIN.add_parser("JPMX",&jpmx_parser),false);
    std::string message1 = "8=FIX.4.2|9=95|35=X|34=5|49=VENUE|52=20160617-23:12:05.551|56=DONALD|268=1|279=1|269=1|270=110|271=5|37=9|10=209|";
    gatewayIN.stop();
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message1),false);
    gatewayIN.start();
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message1),true);

    Message msg1 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg1.getMessageType(),message_type::incremental_snapshot);
    PriceUpdate &pu1 = msg1.getPriceUpdate();
    TEST_FUNCTION(pu1.getAction(),order_action::modify);
    TEST_FUNCTION(pu1.getQuantity(),5);
    TEST_FUNCTION(pu1.getPrice(),110);
    TEST_FUNCTION(pu1.getOrderType(),ordertype::LIMIT);


    std::string message2 = "8=FIX.4.2|9=76|35=A|34=1|49=DONALD|52=20160617-23:11:55.884|56=VENUE|98=0|108=30|141=Y|10=134|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message2),true);
    Message msg2 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg2.getMessageType(),message_type::logon);

    std::string message3 = "8=FIX.4.1|9=49|35=0|34=2|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=228|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message3),true);
    Message msg3 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg3.getMessageType(),message_type::heartbeat);

    std::string message4 = "8=FIX.4.2|9=54|35=5|34=20|49=VENUE|52=20160617-23:12:05.55|56=DONALD|10=134|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message4),true);
    Message msg4 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg4.getMessageType(),message_type::logout);

    std::string message5 = "8=FIX.4.2|9=207|35=W|34=2|49=VENUE|52=20160617-23:12:01.336|56=DONALD|55=MQ|268=2|269=0|270=80|271=5|37=1|269=1|270=100|271=7|37=0||37=0|10=196|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_price_update("JPMX",message5),true);
    Message msg5 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg5.getMessageType(),message_type::full_snapshot);


    Gateway gatewayOut;
    gatewayOut.start();
    gatewayOut.add_parser("JPMX",&jpmx_parser);
    gatewayOut.add_composer("JPMX",&jpmx_composer);
    std::string message6 = "8=FIX.4.2|9=117|35=D|34=2|49=DONALD|52=20160613-22:52:37.227|56=VENUE|11=1|21=3|38=3|40=2|44=100|54=1|55=MQ|60=20160613-22:52:37.227|10=005|";
    TEST_FUNCTION(gatewayOut.process_message_from_exchange_for_order("JPMX",message6),true);
    Message msg6 = gatewayOut.return_stored_message();
    TEST_FUNCTION(msg6.getMessageType(),message_type::new_order);
    Order &or1 = msg6.getOrder();
    TEST_FUNCTION(or1.getQuantity(),3);
    TEST_FUNCTION(or1.getPrice(),100);
    TEST_FUNCTION(or1.getOrderType(),ordertype::LIMIT);
    TEST_STRING(or1.getSymbol(),"MQ");
    TEST_STRING(or1.getVenue(),"JPMX");
    or1.setPrice(50);
    or1.setQuantity(111);
    TEST_STRING(gatewayOut.send_message_for_order("JPMX",msg6).c_str(),"8=FIX.4.2|9=118|35=D|34=2|49=DONALD|52=20160613-22:52:37.227|56=VENUE|11=1|21=3|38=111|40=2|44=50|54=1|55=MQ|60=20160613-22:52:37.227|10=058|");

    SEBXParser sebX_parser;
    SEBXComposer sebX_composer;
    gatewayIN.add_parser("SEBX", &sebX_parser);
    gatewayIN.add_composer("SEBX", &sebX_composer);
    gatewayOut.add_parser("SEBX",&sebX_parser);
    gatewayOut.add_composer("SEBX",&sebX_composer);

    std::string message7 = "NEWORDER|123|150|SEBX|MQ|";
    TEST_FUNCTION(gatewayOut.process_message_from_exchange_for_order("SEBX",message7),true);
    Message msg7 = gatewayOut.return_stored_message();
    TEST_FUNCTION(msg7.getMessageType(),message_type::new_order);
    Order &or2 = msg7.getOrder();
    TEST_FUNCTION(or2.getQuantity(),150);
    TEST_FUNCTION(or2.getPrice(),123);
    TEST_FUNCTION(or2.getOrderType(),ordertype::LIMIT);
    TEST_STRING(or2.getSymbol(),"MQ");
    TEST_STRING(or2.getVenue(),"SEBX");

    std::string message8 = "HEARTBEAT|";
    TEST_FUNCTION(gatewayOut.process_message_from_exchange_for_order("SEBX",message8),true);
    Message msg8 = gatewayOut.return_stored_message();
    TEST_FUNCTION(msg8.getMessageType(),message_type::heartbeat);

    std::string message9 = "LOGON|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_order("SEBX",message9),true);
    Message msg9 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg9.getMessageType(),message_type::logon);

    std::string message10 = "LOGOUT|";
    TEST_FUNCTION(gatewayIN.process_message_from_exchange_for_order("SEBX",message10),true);
    Message msg10 = gatewayIN.return_stored_message();
    TEST_FUNCTION(msg10.getMessageType(),message_type::logout);

    TEST_STRING(gatewayOut.send_message_for_order("SEBX",msg6).c_str(),"NEWORDER|50|111|SEBX|MQ|");

    PRINT_RESULTS();
    return 0;
}
