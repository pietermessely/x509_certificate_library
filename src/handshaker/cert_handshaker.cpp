
#include <iostream>
#include <set>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>


#include <cert/x509.hpp>
#include <cert/cert_helpers.hpp>
#include <cert/cert_handshaker.hpp>

//
// Results from tests are encapsulated in TestResult::value and are inspected  for the correct
// result by a function of the form
//
//      TestResult::validateXXXXX(TestResult::value res, ....)
//
using namespace Cert;
using namespace Cert::Handshaker;

namespace Cert{
namespace Handshaker {

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-variable"

    const int max_read_buffer_length = 1024;
    const bool debug_trace = false;
    #pragma clang diagnostic pop
 
std::string getServerCertificatePem(std::string server, std::string cert_bundle_path)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    ctx.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool
    {
        std::cout << __PRETTY_FUNCTION__  << "Verify callback" << std::endl;
        return preverified;
    });//
    // use a non default root certificate location, AND load them into a custom X509_STORE
    //
    X509_STORE *store = X509_STORE_new();
    X509_STORE_load_locations(store, (const char*)cert_bundle_path.c_str(), NULL);
    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);
    auto v = handshakeWithServer(server, ctx);
    assert(v.is_success());
    return Handshaker::Result::getPem(v);
}
std::string getServerCertificatePem(std::string server, X509_STORE *store)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    ctx.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool
    {
        std::cout << __PRETTY_FUNCTION__  << "Verify callback" << std::endl;
        return preverified;
    });
    SSL_CTX_set_cert_store(ctx.native_handle(), store);

    auto v = handshakeWithServer(server, ctx);
    assert(v.is_success());
    return Handshaker::Result::getPem(v);

}
std::string getServerCertificatePem(std::string server)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    ctx.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool
    {
        std::cout << __PRETTY_FUNCTION__  << "Verify callback" << std::endl;
        return preverified;
    });
    auto df = Cert::Helpers::replace_openssl_get_default_cert_file();
    if (!boost::filesystem::exists(df)) {
        THROW(__func__ << " openssl default cert file does not exist " << df);
    }
    SSL_CTX_load_verify_locations(ctx.native_handle(), df.c_str(), NULL);

    auto v = handshakeWithServer(server, ctx);
    assert(v.is_success());
    return Handshaker::Result::getPem(v);
}
std::string getServerCertificatePem(std::string server, boost::asio::ssl::context& ctx)
{
        auto v = handshakeWithServer(server, ctx);
        assert(v.is_success());
        return Handshaker::Result::getPem(v);
}
Handshaker::Result::Value handshakeWithServer(std::string server, boost::asio::ssl::context& ctx)
{
    boost::asio::io_service io;
    Handshaker::client c("https", server, ctx, io);
    c.handshake([server](boost::system::error_code err) {
        if (err.failed()) {
            std::cout << "handshake callback : " << server << " err: [" << err.message() << "]" << std::endl;
//            assert(false);
        } else {
            std::cout << "handshaker callback : " << server << " success" << std::endl;
        }
    });
    io.run();
    return c.getResult();
}
Handshaker::Result::Value handshakeWithServer(std::string server, std::string cert_bundle_path)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
#define XTURN_OFF_VERIFY
#ifdef TURN_OFF_VERIFY
    ctx.set_verify_mode(boost::asio::ssl::verify_none);
#else
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    ctx.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool
    {
        std::cout << __PRETTY_FUNCTION__  << "Verify callback" << std::endl;
        return preverified;
    });
#endif
    //
    // use a non default root certificate location, AND load them into a custom X509_STORE
    //
    X509_STORE *store = X509_STORE_new();
    X509_STORE_load_locations(store, (const char*)cert_bundle_path.c_str(), NULL);
    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);

    boost::asio::io_service io;
    Handshaker::client c("https", server, ctx, io);
    c.handshake([server](boost::system::error_code err) {
        if (err.failed()) {
            std::cout << "handshake callback : " << server << " err: [" << err.message() << "]" << std::endl;
//            assert(false);
        } else {
            std::cout << "handshaker callback : " << server << " success" << std::endl;
        }
    });
    io.run();
    return c.getResult();
}

 
client::client(
   std::string port,
   std::string server,
   boost::asio::ssl::context &ctx,
   boost::asio::io_service& ios
   ) :   m_port(port), m_server(server), m_ios(ios), m_ctx(ctx), m_socket(m_ios, m_ctx), success(false)
{
//    m_saved_server_certificate = nullptr;
}

client::~client()
{
}

void client::handshake(HandshakeCallback cb)
{
    m_handshakeCallback = cb;
    boost::asio::ip::tcp::resolver resolver(m_ios);
    boost::asio::ip::tcp::resolver::query query(m_server, m_port);
    /**
    * \todo - this resolve operation is synchronous, make it async
    */
    boost::asio::ip::tcp::resolver::iterator endpoints = resolver.resolve(query);

    /**
    * Need this next statement for correct SNI operation
    */
    SSL_set_tlsext_host_name(m_socket.native_handle(), m_server.c_str());
    m_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    /// setup openssl to verify host name
    X509_VERIFY_PARAM *param;
    param = X509_VERIFY_PARAM_new();
    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
    X509_VERIFY_PARAM_set1_host(param, m_server.c_str(), m_server.size());
    SSL_CTX_set1_param(m_ctx.native_handle(), param);
    X509_VERIFY_PARAM_free(param);

    boost::asio::async_connect(
        m_socket.lowest_layer(),
        endpoints,
        boost::bind(&client::p_handle_connect, this, boost::asio::placeholders::error)
    );
    // m_ctx.set_verify_callback([this](bool preverified, boost::asio::ssl::verify_context& ctx) -> bool
    // {
    //     // std::cout << __PRETTY_FUNCTION__ << "Verify callback" << std::endl;
    //     bool b = verify_certificate(preverified, ctx);
    //     return preverified;
    // });
}
#pragma mark - post function
void client::p_postCallback(boost::system::error_code err)
{
    auto c = std::bind(m_handshakeCallback, err);
    m_ios.post(c);
}
#pragma mark - verify callback

bool client::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "XXVerifying " << subject_name << "\n";
    std::cout << "Preverified " << preverified << std::endl;
    //
    // check the server has a certificate
    //
    
    X509* server_cert = SSL_get_peer_certificate(m_socket.native_handle());
    if (server_cert == NULL) {
        return preverified;
    }
    /* get a "list" of alternative names */
    STACK_OF(GENERAL_NAME) *altnames;
    altnames = (STACK_OF(GENERAL_NAME)*) X509_get_ext_d2i(server_cert, NID_subject_alt_name, NULL, NULL);


    return preverified ;
}
#pragma mark - ios handlers
void client::p_handle_connect(const boost::system::error_code& error)
{
    if (!error) {
        m_socket.async_handshake(
            boost::asio::ssl::stream_base::client,
            boost::bind(&client::p_handle_handshake, this, boost::asio::placeholders::error)
        );
    }else{
        this->where = Handshaker::Result::where_connect;
        if (debug_trace) std::cout << "Connect failed: " << error.message() << "\n";
        p_postCallback(error);
    }
}

void client::p_handle_handshake(const boost::system::error_code& error)
{
//    std::cout << "p_handle_handshake: error " << error.message() << std::endl;
    if (!error) {
        int r = SSL_get_verify_result(m_socket.native_handle());
        int r2 = X509_V_OK;

        extractAltNames();
        extractCommonNames();
        saveServerCertificate();
        saveServerCertificateChain();
        this->success = true;
        p_postCallback(error);
    } else {
        this->where = Handshaker::Result::where_handshake;
        if (debug_trace) std::cout << "Handshake failed: " << error.message() << "\n";
        p_postCallback(error);
    }
}
#pragma mark - extractors, collect server info after handshake

void client::extractAltNames()
{
    
    X509* server_cert = SSL_get_peer_certificate(m_socket.native_handle());
    Cert::x509::AlternativeDNSNameSet r = Cert::x509::Cert_GetSubjectAlternativeDNSNames(server_cert);
    m_subjectAltnames = r;
    return;
}
void client::extractCommonNames()
{
    std::vector<std::string> v;
    std::map<std::string, std::string> m;
    /** openssl doc explicitly states the ref count was incremented - hence we have to free it */
    X509* cert = SSL_get_peer_certificate(m_socket.native_handle());
    /**
    * @todo - there is a memory leak here with the cert
    */
    Cert::x509::NameSpecification nspec = Cert::x509::Cert_GetSubjectNameAsSpec(cert);
//    _subject_name_spec = nspec;
    X509_NAME* name = X509_get_subject_name(cert);
    int i = -1;
    ASN1_STRING* common_name = 0;
    std::string commonNameString;
    while ((i = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0)
    {
        X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(name, i);
        common_name = X509_NAME_ENTRY_get_data(name_entry);
        std::string sname((char*)common_name->data, (int)common_name->length);
        m_commonNames.insert(sname);
        commonNameString = sname;
        v.push_back(sname);
        m[sname] = sname;
    }
    if (common_name && common_name->data && common_name->length)
    {
        m_lastCommonName = commonNameString;
    }
    auto s = Cert::x509::Cert_GetSubjectNameAsSpec(cert);
    if( cert != NULL)
        X509_free(cert); // see note above about needing to free
    auto s23 = s;
}

void client::saveServerCertificate()
{
    // this call ups the ref count on the return value
    X509* cert = SSL_get_peer_certificate(m_socket.native_handle());
    m_saved_server_certificate_pem = Cert::x509::Cert_PEMString(cert);
    m_saved_certificate = Certificate(cert);
    // since getting the peer cert upped the ref count we need to free it
    X509_free(cert);
}

void client::saveServerCertificateChain()
{
    /// from what I can see in the openssl code the ref count is not upped
    STACK_OF(X509)* cert_chain = SSL_get_peer_cert_chain(m_socket.native_handle());
    x509::CertChain cc = Cert::x509::CertChain_FromStack(cert_chain);
    m_saved_certificate_chain = cc;
    m_pem_saved_certificate_chain.insert(m_pem_saved_certificate_chain.end(), cc.begin(), cc.end());
    // should not free - ref count not upped when get_peer)certificate_chain
    // and if free it get a crash
    // sk_X509_free(cert_chain);

    /**
    * @todo - once we use openssl 1.1.0 we can also ssl_get0_verified_chain()
    *
    * @note - we do NOT need to free the stack as the ref count was not incremented. or so says Openssl DOCS
    */

    return;
}

#pragma mark - getters

Handshaker::Result::Value client::getResult()
{
    return Handshaker::Result::makeValue(
        this->success,
        this->where,
        this->m_subjectAltnames,
        m_saved_server_certificate_pem,
        m_pem_saved_certificate_chain
    );
}

} // namespace Handshaker
} // namespace Cert
