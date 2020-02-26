//
//  x509_conf.cpp
//  openssl_10_6
//
//  Created by ROBERT BLACKWELL on 10/26/17.
//  Copyright © 2017 Blackwellapps. All rights reserved.
//
#include <string>
#include <cert/x509.hpp>

Cert::x509::Conf::Conf(std::string filename)
{
    long err;

    _conf = NCONF_new (NCONF_default ());
    /**
    ** load the cnf file
    **/
    if (!NCONF_load (_conf, filename.c_str(), &err))
    {
        if (err == 0) {
            X509_TRIGGER_ERROR("Error opening configuration file");
        } else {
            X509_TRIGGER_ERROR("Errors parsing configuration file");
        }
    }
}

Cert::x509::Conf::~Conf()
{
    NCONF_free(_conf);
}
std::string Cert::x509::Conf::getGlobalString(std::string key)
{
    const char* c_key = key.c_str();
    char* s_val;
    /**
    ** get the value (string) of the variable "GlobalVar" - example of how to find the string value
    ** of a parameter that is NOT inside a section
    **/
    if (!(s_val = NCONF_get_string (_conf, NULL, c_key))) {
        X509_TRIGGER_ERROR("Error finding string");
    }
    std::string value(s_val);
    return value;
}
int Cert::x509::Conf::getGlobalNumber(std::string key)
{
    const char* c_key = key.c_str();
    long i_val;
    long err;
    
    /**
    ** Get the value (number) of variable GlobalNum from the global section - depends on version
    **/
#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
    if (!(err = NCONF_get_number_e (_conf, NULL, c_key, &i_val))) {
        X509_TRIGGER_ERROR("Error finding number");
    }
#else
    if (!(s_val = NCONF_get_string (_conf, NULL, c_key))) {
        X509_TRIGGER_ERROR("Error finding number");
    }
    i_val = atoi (s_val);
#endif
    return (int)i_val;
}

std::string Cert::x509::Conf::getSectionString(std::string section, std::string key)
{
    char* s_val;
    const char* c_key = key.c_str();
    const char* c_section = section.c_str();
    /**
    ** Get the value of the variable PARAM(Params) that is inside the section names SEC_NAME(SectionName)
    **
    */
    if (!(s_val = NCONF_get_string (_conf, c_key, c_section))) {
        X509_TRIGGER_ERROR("Error finding string");
    }
    std::string value(s_val);
    return value;
}


std::map<std::string, std::string>
Cert::x509::Conf::getAllSectionStringValues(std::string section)
{
    const char* c_section = section.c_str();
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-variable"
    const char* value;
    #pragma clang diagnostic pop
    STACK_OF (CONF_VALUE) * sec;
    CONF_VALUE *item;
    int i;
    std::map<std::string, std::string> res;

    if (!(sec = NCONF_get_section (_conf, c_section))) {
//    fprintf (stderr, "Error finding [%s]\n", key);
        X509_TRIGGER_ERROR("Error finding section: " + section);
    }

    /**
    ** Then iterate through all those key/value pairs and print them out
    */
    for (i = 0; i < sk_CONF_VALUE_num (sec); i++) {
        item = sk_CONF_VALUE_value (sec, i);
        std::string k(item->name);
        std::string v(item->value);
        res[k] = v;
    }
    return res;
}
