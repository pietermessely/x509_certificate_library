#ifndef certlib_oo_cert_evp_pkey_hpp
#define certlib_oo_cert_evp_pkey_hpp
#include <cert/constants.hpp>
#include <cert/version_check.hpp>
#include <cert/x509.hpp>

namespace Cert{
class EvpPKey;
class EvpPKey
{
    public:
    EvpPKey();
	EvpPKey(boost::filesystem::path filePath, std::string password);
	EvpPKey(std::string pem);
	EvpPKey(EVP_PKEY* pkey);

	explicit operator bool() const;
//    EvpPKey(EvpPKey&& other) = delete;
//    EvpPKey& operator=(EvpPKey&& other) = delete;

    EvpPKey(const EvpPKey& other) = default;
    EvpPKey & operator=(const EvpPKey&) = default;
    
    ~EvpPKey();
    EVP_PKEY* Rsa_Generate();

#pragma mark - pkeys, EVP_PKEY*
    // The pointer returned by this method is owned by the class instance. Do not try
    // and free it.
    EVP_PKEY* native();
    std::string privateKeyAsPemString();
    void writePrivateKeyToFile(boost::filesystem::path filePath);
    std::string printPrivateKeyToString();
    void printPrivateKeyToFile(boost::filesystem::path filePath);
    
    std::string publicKeyToString();

    private:
    class Impl;
    std::shared_ptr<Impl>   m_impl_sptr;
};

}

#endif
