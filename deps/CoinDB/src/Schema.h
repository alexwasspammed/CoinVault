///////////////////////////////////////////////////////////////////////////////
//
// Schema.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//

// odb compiler complains about #pragma once
#ifndef COINDB_SCHEMA_H
#define COINDB_SCHEMA_H

#include <CoinCore/CoinNodeData.h>

#include <CoinQ/CoinQ_typedefs.h>
#include <CoinQ/CoinQ_blocks.h>

#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <odb/database.hxx>

#include <memory>

// support for boost serialization
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>

#include <logger/logger.h>

#pragma db namespace session
namespace CoinDB
{

typedef odb::nullable<unsigned long> null_id_t;

#pragma db value(bytes_t) type("BLOB")

////////////////////
// SCHEMA VERSION //
////////////////////

#define SCHEMA_BASE_VERSION 4
#define SCHEMA_VERSION      5

#ifdef ODB_COMPILER
#pragma db model version(SCHEMA_BASE_VERSION, SCHEMA_VERSION, open)
#endif

#pragma db object pointer(std::shared_ptr)
class Version
{
public:
    Version(uint32_t version = SCHEMA_VERSION) : version_(version) { }

    unsigned long id() const { return id_; }

    void version(uint32_t version) { version_ = version; }
    uint32_t version() const { return version_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint32_t version_;
};


////////////////////////////
// KEYCHAINS AND ACCOUNTS //
////////////////////////////

class Account;
class AccountBin;
class SigningScript;

#pragma db object pointer(std::shared_ptr)
class Keychain : public std::enable_shared_from_this<Keychain>
{
public:
    Keychain(bool hidden = false) : hidden_(hidden) { }
    Keychain(const std::string& name, const secure_bytes_t& entropy = secure_bytes_t(), const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t()); // Creates a new root keychain
    Keychain(const Keychain& source)
        : name_(source.name_), depth_(source.depth_), parent_fp_(source.parent_fp_), child_num_(source.child_num_), pubkey_(source.pubkey_), chain_code_(source.chain_code_), chain_code_ciphertext_(source.chain_code_ciphertext_), chain_code_salt_(source.chain_code_salt_), privkey_(source.privkey_), privkey_ciphertext_(source.privkey_ciphertext_), privkey_salt_(source.privkey_salt_), parent_(source.parent_), derivation_path_(source.derivation_path_), hidden_(source.hidden_) { }

    Keychain& operator=(const Keychain& source);

    std::shared_ptr<Keychain> get_shared_ptr() { return shared_from_this(); }

    unsigned int id() const { return id_; }

    std::string name() const { return name_; }
    void name(const std::string& name) { name_ = name; }

    std::shared_ptr<Keychain> root() { return (parent_ ? parent_->root() : shared_from_this()); }
    std::shared_ptr<Keychain> parent() const { return parent_; }
    std::shared_ptr<Keychain> child(uint32_t i, bool get_private = false);

    const std::vector<uint32_t>& derivation_path() const { return derivation_path_; }

    bool isPrivate() const { return (!privkey_.empty()) || (!privkey_ciphertext_.empty()); }
    bool isEncrypted() const { return !privkey_salt_.empty(); }

    // Lock keys must be set before persisting
    bool setPrivateKeyUnlockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());
    bool setChainCodeUnlockKey(const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());

    void lockPrivateKey() const;
    void lockChainCode() const;
    void lockAll() const;

    bool isPrivateKeyLocked() const;
    bool isChainCodeLocked() const;

    bool unlockPrivateKey(const secure_bytes_t& lock_key) const;
    bool unlockChainCode(const secure_bytes_t& lock_key) const;

    secure_bytes_t getSigningPrivateKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;
    bytes_t getSigningPublicKey(uint32_t i, const std::vector<uint32_t>& derivation_path = std::vector<uint32_t>()) const;

    uint32_t depth() const { return depth_; }
    uint32_t parent_fp() const { return parent_fp_; }
    uint32_t child_num() const { return child_num_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t chain_code() const;

    const bytes_t& chain_code_ciphertext() const { return chain_code_ciphertext_; }
    const bytes_t& chain_code_salt() const { return chain_code_salt_; }

    void importPrivateKey(const Keychain& source);

    // hash = ripemd160(sha256(pubkey + chain_code))
    const bytes_t& hash() const { return hash_; }

    void hidden(bool hidden) { hidden_ = hidden; }
    bool hidden() const { return hidden_; }

    void extkey(const secure_bytes_t& extkey, bool try_private = true, const secure_bytes_t& lock_key = secure_bytes_t(), const bytes_t& salt = bytes_t());
    secure_bytes_t extkey(bool get_private = false) const;

    void clearPrivateKey();

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;

    uint32_t depth_;
    uint32_t parent_fp_;
    uint32_t child_num_;
    bytes_t pubkey_;

    #pragma db transient
    mutable secure_bytes_t chain_code_;
    bytes_t chain_code_ciphertext_;
    bytes_t chain_code_salt_;

    #pragma db transient
    mutable secure_bytes_t privkey_;
    bytes_t privkey_ciphertext_;
    bytes_t privkey_salt_;

    #pragma db null
    std::shared_ptr<Keychain> parent_;
    std::vector<uint32_t> derivation_path_;

    #pragma db value_not_null inverse(parent_)
    std::vector<std::weak_ptr<Keychain>> children_;

    bytes_t hash_;

    bool hidden_; // whether to display in UI

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & name_;
        ar & hash_;
        ar & depth_;
        ar & parent_fp_;
        ar & child_num_;
        ar & pubkey_;
        ar & chain_code_ciphertext_;
        ar & chain_code_salt_;
        ar & privkey_ciphertext_;
        ar & privkey_salt_;
    }
};

typedef std::set<std::shared_ptr<Keychain>> KeychainSet;


#pragma db object pointer(std::shared_ptr)
class Key
{
public:
    Key(const std::shared_ptr<Keychain>& keychain, uint32_t index);

    unsigned long id() const { return id_; }
    const bytes_t& pubkey() const { return pubkey_; }
    secure_bytes_t privkey() const;
    secure_bytes_t try_privkey() const;
    bool isPrivate() const { return is_private_; }


    std::shared_ptr<Keychain> root_keychain() const { return root_keychain_; }
    std::vector<uint32_t> derivation_path() const { return derivation_path_; }
    uint32_t index() const { return index_; }

    void updatePrivate() { is_private_ = root_keychain_->isPrivate(); }

private:
    friend class odb::access;

    Key() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Keychain> root_keychain_;
    std::vector<uint32_t> derivation_path_;
    uint32_t index_;

    bytes_t pubkey_;
    bool is_private_;
};

typedef std::vector<std::shared_ptr<Key>> KeyVector;



const std::string CHANGE_BIN_NAME = "@change";
const std::string DEFAULT_BIN_NAME = "@default";

#pragma db object pointer(std::shared_ptr)
class AccountBin : public std::enable_shared_from_this<AccountBin>
{
public:
    // Note: index 0 is reserved for subaccounts but vector indices are 0-indexed so an offset of 1 is required.
    enum
    {
        CHANGE_INDEX = 1,
        DEFAULT_INDEX,
        FIRST_CUSTOM_INDEX
    };

    AccountBin() { }
    AccountBin(std::shared_ptr<Account> account, uint32_t index, const std::string& name);
    AccountBin(const AccountBin& source) : account_(source.account_), index_(source.index_), name_(source.name_), script_count_(source.script_count_), next_script_index_(source.next_script_index_), minsigs_(source.minsigs_), keychains_(source.keychains_), keychains__(source.keychains__) { }

    unsigned long id() const { return id_; }

    void account(std::shared_ptr<Account> account) { account_ = account; }
    std::shared_ptr<Account> account() const { return account_.lock(); }

    std::string account_name() const;

    uint32_t index() const { return index_; }

    void name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    uint32_t script_count() const { return script_count_; }
    uint32_t next_script_index() const { return next_script_index_; }

    uint32_t minsigs() const { return minsigs_; }

    std::shared_ptr<SigningScript> newSigningScript(const std::string& label = "");
    void markSigningScriptIssued(uint32_t script_index);

    void keychains(const KeychainSet& keychains) { keychains_ = keychains; keychains__ = keychains; } // only used for imported account bins
    const KeychainSet& keychains() const { loadKeychains(); return keychains__; }

    bool isChange() const { return index_ == CHANGE_INDEX; }
    bool isDefault() const { return index_ == DEFAULT_INDEX; }

    void makeExport(const std::string& name);
    void makeImport();

    void updateHash();
    const bytes_t& hash() const { return hash_; }

private:
    void loadKeychains() const;

    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db null
    std::weak_ptr<Account> account_;
    uint32_t index_;
    std::string name_;

    uint32_t script_count_;
    uint32_t next_script_index_; // index of next script in pool that will be issued

    uint32_t minsigs_;

    // Keychains are only stored in database for nonderived keychains. Otherwise, they are transient and loaded only via loadKeychains().
    #pragma db value_not_null
    KeychainSet keychains_;
    #pragma db transient
    mutable KeychainSet keychains__;

    #pragma db unique
    bytes_t hash_; // ripemd160(sha256(data)) where data = concat(first byte(minsigs), keychain hash 1, keychain hash 2, ...) and keychain hashes are sorted lexically

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int /*version*/) const
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
        ar & minsigs_;

        uint32_t n = keychains_.size();
        ar & n;
        for (auto& keychain: keychains_)    { ar & *keychain; }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    {
        ar & name_;
        ar & index_;
        ar & next_script_index_;
        ar & minsigs_;

        uint32_t n;
        ar & n;
        keychains_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<Keychain> keychain(new Keychain(true)); // Load these keychains as hidden keychains.
            ar & *keychain;
            keychains_.insert(keychain);
        }
        keychains__ = keychains_;
        script_count_ = 0;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

typedef std::vector<std::shared_ptr<AccountBin>> AccountBinVector;

// Immutable object containng keychain and bin names as strings
class AccountInfo
{
public:
    AccountInfo(
        unsigned long id,
        const std::string& name,
        unsigned int minsigs,
        const std::vector<std::string>& keychain_names,
        uint32_t unused_pool_size,
        uint32_t time_created,
        const std::vector<std::string>& bin_names
    ) :
        id_(id),
        name_(name),
        minsigs_(minsigs),
        keychain_names_(keychain_names),
        unused_pool_size_(unused_pool_size),
        time_created_(time_created),
        bin_names_(bin_names)
    {
        std::sort(keychain_names_.begin(), keychain_names_.end());
    }

    AccountInfo(const AccountInfo& source) :
        id_(source.id_),
        name_(source.name_),
        minsigs_(source.minsigs_),
        keychain_names_(source.keychain_names_),
        unused_pool_size_(source.unused_pool_size_),
        time_created_(source.time_created_),
        bin_names_(source.bin_names_)
    { }

    AccountInfo& operator=(const AccountInfo& source)
    {
        id_ = source.id_;
        name_ = source.name_;
        minsigs_ = source.minsigs_;
        keychain_names_ = source.keychain_names_;
        unused_pool_size_ = source.unused_pool_size_;
        time_created_ = source.time_created_;
        bin_names_ = source.bin_names_;
        return *this;
    }

    unsigned int                        id() const { return id_; }
    const std::string&                  name() const { return name_; }
    unsigned int                        minsigs() const { return minsigs_; }
    const std::vector<std::string>&     keychain_names() const { return keychain_names_; }
    uint32_t                            unused_pool_size() const { return unused_pool_size_; }
    uint32_t                            time_created() const { return time_created_; }
    const std::vector<std::string>&     bin_names() const { return bin_names_; }

private:
    unsigned long               id_;
    std::string                 name_;
    unsigned int                minsigs_;
    std::vector<std::string>    keychain_names_;
    uint32_t                    unused_pool_size_;
    uint32_t                    time_created_;
    std::vector<std::string>    bin_names_;
};

const uint32_t DEFAULT_UNUSED_POOL_SIZE = 25;

#pragma db object pointer(std::shared_ptr)
class Account : public std::enable_shared_from_this<Account>
{
public:
    Account() { }
    Account(const std::string& name, unsigned int minsigs, const KeychainSet& keychains, uint32_t unused_pool_size = DEFAULT_UNUSED_POOL_SIZE, uint32_t time_created = time(NULL));

    void updateHash();

    AccountInfo accountInfo() const;

    unsigned long id() const { return id_; }

    void name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }
    unsigned int minsigs() const { return minsigs_; }


    void keychains(const KeychainSet& keychains) { keychains_ = keychains; }
    KeychainSet keychains() const { return keychains_; }

    uint32_t unused_pool_size() const { return unused_pool_size_; }
    uint32_t time_created() const { return time_created_; }
    const bytes_t& hash() const { return hash_; }
    AccountBinVector bins() const { return bins_; }

    std::shared_ptr<AccountBin> addBin(const std::string& name);

    uint32_t bin_count() const { return bins_.size(); }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    std::string name_;
    unsigned int minsigs_;

    #pragma db value_not_null \
        id_column("object_id") value_column("value")
    KeychainSet keychains_;

    uint32_t unused_pool_size_; // how many unused scripts we want in our lookahead
    uint32_t time_created_;

    #pragma db unique
    bytes_t hash_; // ripemd160(sha256(data)) where data = concat(first byte(minsigs), keychain hash 1, keychain hash 2, ...) and keychain hashes are sorted lexically

    #pragma db value_not_null inverse(account_)
    AccountBinVector bins_;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int /*version*/) const
    {
        ar & name_;
        ar & minsigs_;

        uint32_t n = keychains_.size();
        ar & n;
        for (auto& keychain: keychains_)    { ar & *keychain; }

        ar & unused_pool_size_;
        ar & time_created_;

        n = bins_.size();
        ar & n;
        for (auto& bin: bins_)              { ar & *bin; }
    }
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    {
        ar & name_;
        ar & minsigs_;

        uint32_t n;
        ar & n;
        keychains_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<Keychain> keychain(new Keychain());
            ar & *keychain;
            keychains_.insert(keychain);
        }

        ar & unused_pool_size_;
        ar & time_created_;

        // TODO: validate bins
        ar & n;
        bins_.clear();
        for (uint32_t i = 0; i < n; i++)
        {
            std::shared_ptr<AccountBin> bin(new AccountBin());
            ar & *bin;
            bin->account(shared_from_this());
            bins_.push_back(bin);
        }

        updateHash();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


#pragma db object pointer(std::shared_ptr)
class SigningScript : public std::enable_shared_from_this<SigningScript>
{
public:
    enum status_t
    {
        NONE        =  0,
        UNUSED      =  1,
        CHANGE      =  1 << 1,
        ISSUED      =  1 << 2,
        USED        =  1 << 3,
        ALL         = (1 << 4) - 1
    };

    static std::string              getStatusString(int status);
    static std::vector<status_t>    getStatusFlags(int status);

    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const std::string& label = "", status_t status = UNUSED);
    SigningScript(std::shared_ptr<AccountBin> account_bin, uint32_t index, const bytes_t& txinscript, const bytes_t& txoutscript, const std::string& label = "", status_t status = UNUSED)
        : account_(account_bin->account()), account_bin_(account_bin), index_(index), label_(label), status_(status), txinscript_(txinscript), txoutscript_(txoutscript) { }

    unsigned long id() const { return id_; }
    void label(const std::string& label) { label_ = label; }
    const std::string label() const { return label_; }

    void status(status_t status);
    status_t status() const { return status_; }

    const bytes_t& txinscript() const { return txinscript_; }
    const bytes_t& txoutscript() const { return txoutscript_; }

    std::shared_ptr<Account> account() const { return account_; }

    // 0 is reserved for subaccounts, 1 is reserved for change addresses, 2 is reserved for the default bin.
    std::shared_ptr<AccountBin> account_bin() const { return account_bin_; }

    uint32_t index() const { return index_; }

    KeyVector& keys() { return keys_; }

private:
    friend class odb::access;
    SigningScript() { }

    #pragma db id auto
    unsigned long id_;

    #pragma db value_not_null
    std::shared_ptr<Account> account_;

    #pragma db value_not_null
    std::shared_ptr<AccountBin> account_bin_;
    uint32_t index_;

    std::string label_;
    status_t status_;

    bytes_t txinscript_; // unsigned (0 byte length placeholders are used for signatures)
    bytes_t txoutscript_;

    KeyVector keys_;
};


/////////////////////////////
// BLOCKS AND TRANSACTIONS //
/////////////////////////////

class Tx;

#pragma db object pointer(std::shared_ptr)
class BlockHeader
{
public:
    BlockHeader() { }

    BlockHeader(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff) { fromCoinCore(blockheader, height); }

    BlockHeader(const bytes_t& hash, uint32_t height, uint32_t version, const bytes_t& prevhash, const bytes_t& merkleroot, uint32_t timestamp, uint32_t bits, uint32_t nonce)
    : hash_(hash), height_(height), version_(version), prevhash_(prevhash), merkleroot_(merkleroot), timestamp_(timestamp), bits_(bits), nonce_(nonce) { }

    void fromCoinCore(const Coin::CoinBlockHeader& blockheader, uint32_t height = 0xffffffff);
    Coin::CoinBlockHeader toCoinCore() const;

    unsigned long id() const { return id_; }
    bytes_t hash() const { return hash_; }

    void height(uint32_t height) { height_ = height; }
    uint32_t height() const { return height_; }

    uint32_t version() const { return version_; }
    bytes_t prevhash() const { return prevhash_; }
    bytes_t merkleroot() const { return merkleroot_; }
    uint32_t timestamp() const { return timestamp_; }
    uint32_t bits() const { return bits_; }
    uint32_t nonce() const { return nonce_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db unique
    bytes_t hash_;

    #pragma db unique
    uint32_t height_;

    uint32_t version_;
    bytes_t prevhash_;
    bytes_t merkleroot_;
    uint32_t timestamp_;
    uint32_t bits_;
    uint32_t nonce_;
};


#pragma db object pointer(std::shared_ptr)
class MerkleBlock
{
public:
    MerkleBlock() { }

    MerkleBlock(const std::shared_ptr<BlockHeader>& blockheader, uint32_t txcount, const std::vector<bytes_t>& hashes, const bytes_t& flags)
        : blockheader_(blockheader), txcount_(txcount), hashes_(hashes), flags_(flags) { }

    MerkleBlock(const ChainMerkleBlock& merkleblock) { fromCoinCore(merkleblock, merkleblock.height); }

    void fromCoinCore(const Coin::MerkleBlock& merkleblock, uint32_t height = 0xffffffff);
    Coin::MerkleBlock toCoinCore() const;

    unsigned long id() const { return id_; }

    // The logic of blockheader management and persistence is handled by the user of this class.
    void blockheader(const std::shared_ptr<BlockHeader>& blockheader) { blockheader_ = blockheader; }
    const std::shared_ptr<BlockHeader>& blockheader() const { return blockheader_; }

    void txcount(uint32_t txcount) { txcount_ = txcount; }
    uint32_t txcount() const { return txcount_; }

    void hashes(const std::vector<bytes_t>& hashes) { hashes_ = hashes; }
    const std::vector<bytes_t>& hashes() const { return hashes_; }

    void flags(const bytes_t& flags) { flags_ = flags; }
    const bytes_t& flags() const { return flags_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    #pragma db not_null
    std::shared_ptr<BlockHeader> blockheader_;

    uint32_t txcount_;

    #pragma db value_not_null \
        id_column("object_id") value_column("value")
    std::vector<bytes_t> hashes_;

    bytes_t flags_;
};


#pragma db object pointer(std::shared_ptr)
class TxIn
{
public:
    TxIn(const bytes_t& outhash, uint32_t outindex, const bytes_t& script, uint32_t sequence)
        : outhash_(outhash), outindex_(outindex), script_(script), sequence_(sequence) { }

    TxIn(const Coin::TxIn& coin_txin);
    TxIn(const bytes_t& raw);

    Coin::TxIn toCoinCore() const;

    unsigned long id() const { return id_; }
    const bytes_t& outhash() const { return outhash_; }
    uint32_t outindex() const { return outindex_; }

    void script(const bytes_t& script) { script_ = script; }
    const bytes_t& script() const { return script_; }

    uint32_t sequence() const { return sequence_; }
    bytes_t raw() const;

    void tx(std::shared_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(unsigned int txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

private:
    friend class odb::access;
    TxIn() { }

    #pragma db id auto
    unsigned long id_;

    bytes_t outhash_;
    uint32_t outindex_;
    bytes_t script_;
    uint32_t sequence_;

    #pragma db not_null
    std::weak_ptr<Tx> tx_;

    uint32_t txindex_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & outhash_;
        ar & outindex_;
        ar & script_;
        ar & sequence_;
    }
};

typedef std::vector<std::shared_ptr<TxIn>> txins_t;


#pragma db object pointer(std::shared_ptr)
class TxOut
{
public:
    enum status_t
    {
        NEITHER     =  0,
        UNSPENT     =  1,
        SPENT       =  1 << 1,
        BOTH        = (1 << 2) - 1
    };

    static std::string              getStatusString(int flags);
    static std::vector<status_t>    getStatusFlags(int flags);


    enum role_t
    {
        ROLE_NONE       =  0,
        ROLE_SENDER     =  1,
        ROLE_RECEIVER   =  1 << 1,
        ROLE_BOTH       = (1 << 2) - 1
    };

    static std::string              getRoleString(int flags);
    static std::vector<role_t>      getRoleFlags(int flags);


    TxOut() : status_(UNSPENT) { }
    TxOut(uint64_t value, const bytes_t& script)
        : value_(value), script_(script), status_(UNSPENT) { }

    // Constructor for change and transfers
    TxOut(uint64_t value, std::shared_ptr<SigningScript> signingscript);

    TxOut(const Coin::TxOut& coin_txout);
    TxOut(const bytes_t& raw);

    Coin::TxOut toCoinCore() const;

    unsigned long id() const { return id_; }
    uint64_t value() const { return value_; }
    const bytes_t& script() const { return script_; }
    bytes_t raw() const;

    void tx(std::shared_ptr<Tx> tx) { tx_ = tx; }
    const std::shared_ptr<Tx> tx() const { return tx_.lock(); }

    void txindex(uint32_t txindex) { txindex_ = txindex; }
    uint32_t txindex() const { return txindex_; }

    void spent(std::shared_ptr<TxIn> spent);
    const std::shared_ptr<TxIn> spent() const { return spent_; }

    void sending_account(std::shared_ptr<Account> sending_account) { sending_account_ = sending_account; }
    const std::shared_ptr<Account> sending_account() const { return sending_account_; }

    void sending_label(const std::string& label) { sending_label_ = label; }
    const std::string& sending_label() const { return sending_label_; }

    const std::shared_ptr<Account> receiving_account() const { return receiving_account_; }

    void receiving_label(const std::string& label) { receiving_label_ = label; }
    const std::string& receiving_label() const { return receiving_label_; }

    const std::shared_ptr<AccountBin> account_bin() const { return account_bin_; }

    void signingscript(std::shared_ptr<SigningScript> signingscript);
    const std::shared_ptr<SigningScript> signingscript() const { return signingscript_; }

    status_t status() const { return status_; }

private:
    friend class odb::access;

    #pragma db id auto
    unsigned long id_;

    uint64_t value_;
    bytes_t script_;

    #pragma db not_null
    std::weak_ptr<Tx> tx_;
    uint32_t txindex_;

    #pragma db null
    std::shared_ptr<TxIn> spent_;

    #pragma db null
    std::shared_ptr<Account> sending_account_;
    std::string sending_label_;

    #pragma db null
    std::shared_ptr<Account> receiving_account_;
    std::string receiving_label_;

    // account_bin and signingscript are only for receiving account
    #pragma db null
    std::shared_ptr<AccountBin> account_bin_;

    #pragma db null
    std::shared_ptr<SigningScript> signingscript_;

    // status == SPENT if spent_ is not null. Otherwise UNSPENT.
    // Redundant but convenient for view queries.
    status_t status_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & value_;
        ar & script_;
        ar & sending_label_;
        ar & receiving_label_;
    }
};

typedef std::vector<std::shared_ptr<TxOut>> txouts_t;


#pragma db object pointer(std::shared_ptr)
class Tx : public std::enable_shared_from_this<Tx>
{
public:
    /*
     * If status is UNSIGNED, remove all txinscripts before hashing so hash
     * is unchanged by adding signatures until fully signed. Then compute
     * normal hash and transition to one of the other states.
     *
     * The states are ordered such that transitions are generally from smaller values to larger values.
     * Blockchain reorgs are the exception where it's possible that a CONFIRMED state reverts to an
     * earlier state.
     */
    enum status_t
    {
        NO_STATUS    =  0,
        UNSIGNED     =  1,      // still missing signatures
        UNSENT       =  1 << 1, // signed but not yet broadcast to network
        SENT         =  1 << 2, // sent to at least one peer but possibly not propagated
        PROPAGATED   =  1 << 3, // received from at least one peer
        CONFLICTING  =  1 << 4, // unconfirmed and spends the same output as another transaction
        CANCELED     =  1 << 5, // either will never be broadcast or will never confirm
        CONFIRMED    =  1 << 6, // exists in blockchain
        ALL          = (1 << 7) - 1
    };

    static std::string              getStatusString(int status);
    static std::vector<status_t>    getStatusFlags(int status);

    Tx(uint32_t version = 1, uint32_t locktime = 0, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED)
        : version_(version), locktime_(locktime), timestamp_(timestamp), status_(status), have_fee_(false), fee_(0) { }

    void set(uint32_t version, const txins_t& txins, const txouts_t& txouts, uint32_t locktime, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED);
    void set(Coin::Transaction coin_tx, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED);
    void set(const bytes_t& raw, uint32_t timestamp = 0xffffffff, status_t status = PROPAGATED);

    Coin::Transaction toCoinCore() const;

    void setBlock(std::shared_ptr<BlockHeader> blockheader, uint32_t blockindex);

    unsigned long id() const { return id_; }
    uint32_t version() const { return version_; }
    const bytes_t& hash() const { return hash_; }
    bytes_t unsigned_hash() const { return unsigned_hash_; }
    txins_t txins() const { return txins_; }
    txouts_t txouts() const { return txouts_; }
    uint32_t locktime() const { return locktime_; }
    bytes_t raw() const;

    void timestamp(uint32_t timestamp) { timestamp_ = timestamp; }
    uint32_t timestamp() const { return timestamp_; }

    bool updateStatus(status_t status = NO_STATUS); // Will keep the status it already had if it didn't change and no parameter is passed. Returns true iff status changed.
    status_t status() const { return status_; }

    void fee(uint64_t fee) { have_fee_ = true; fee_ = fee; }
    uint64_t fee() const { return fee_; }

    bool have_fee() const { return have_fee_; }

    void block(std::shared_ptr<BlockHeader> header, uint32_t index) { blockheader_ = header, blockindex_ = index; }

    void blockheader(std::shared_ptr<BlockHeader> blockheader);
    std::shared_ptr<BlockHeader> blockheader() const { return blockheader_; }

    void shuffle_txins();
    void shuffle_txouts();

    unsigned int missingSigCount() const;
    std::set<bytes_t> missingSigPubkeys() const;

private:
    friend class odb::access;

    void fromCoinCore(const Coin::Transaction& coin_tx);

    #pragma db id auto
    unsigned long id_;

    // hash stays empty until transaction is fully signed.
    bytes_t hash_;

    // We'll use the unsigned hash as a unique identifier to avoid malleability issues.
    #pragma db unique
    bytes_t unsigned_hash_;

    uint32_t version_;

    #pragma db value_not_null inverse(tx_)
    txins_t txins_;

    #pragma db value_not_null inverse(tx_)
    txouts_t txouts_;

    uint32_t locktime_;

    // Timestamp should be set each time we modify the transaction.
    // Once status is PROPAGATED the timestamp is fixed.
    // Timestamp defaults to 0xffffffff
    uint32_t timestamp_;

    status_t status_;

    // Due to issue with odb::nullable<uint64_t> in mingw-w64, we use a second bool variable.
    bool have_fee_;
    uint64_t fee_;

    #pragma db null
    std::shared_ptr<BlockHeader> blockheader_;

    #pragma db null
    odb::nullable<uint32_t> blockindex_;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
        ar & version_;
        ar & txins_;
        ar & txouts_;
        ar & locktime_;
        ar & timestamp_; // only used for sorting in UI
    }
};


// Views
#pragma db view \
    object(Keychain) \
    table("Account_keychains" = "t": "t.value = " + Keychain::id_) \
    object(Account: "t.object_id = " + Account::id_)
struct KeychainView
{
    KeychainView() : is_locked(true) { }

    #pragma db column(Keychain::id_)
    unsigned long id;
    #pragma db column(Keychain::name_)
    std::string name;
    #pragma db column(Keychain::depth_)
    uint32_t depth;
    #pragma db column(Keychain::parent_fp_)
    uint32_t parent_fp;
    #pragma db column(Keychain::child_num_)
    uint32_t child_num;
    #pragma db column(Keychain::pubkey_)
    bytes_t pubkey;
    #pragma db column(Keychain::hash_)
    bytes_t hash;
    #pragma db column("length(" + Keychain::privkey_ciphertext_ + ") != 0")
    bool is_private;
    #pragma db column("length(" + Keychain::privkey_salt_ + ") != 0")
    bool is_encrypted;
    #pragma db transient
    bool is_locked;
};

#pragma db view \
    object(Account)
struct AccountView
{
    unsigned long               id;
    std::string                 name;
    unsigned int                minsigs;
    uint32_t                    unused_pool_size;
    uint32_t                    time_created;

    #pragma db transient
    std::vector<KeychainView>    keychain_views;

    #pragma db transient
    std::vector<std::string>    bin_names;
};

#pragma db view \
    object(AccountBin) \
    object(Account: AccountBin::account_)
struct AccountBinView
{
    #pragma db column(Account::id_)
    unsigned long account_id;
    #pragma db column(Account::name_)
    std::string account_name;
    #pragma db column(Account::hash_)
    bytes_t account_hash;

    #pragma db column(AccountBin::id_)
    unsigned long bin_id; 
    #pragma db column(AccountBin::name_)
    std::string bin_name;
    #pragma db column(AccountBin::hash_)
    bytes_t bin_hash;
};

#pragma db view \
    object(SigningScript) \
    object(Account: SigningScript::account_) \
    object(AccountBin: SigningScript::account_bin_)
struct SigningScriptView
{
    #pragma db column(Account::id_)
    unsigned long account_id;

    #pragma db column(Account::name_)
    std::string account_name;

    #pragma db column(AccountBin::id_)
    unsigned long account_bin_id;

    #pragma db column(AccountBin::name_)
    std::string account_bin_name;

    #pragma db column(SigningScript::id_)
    unsigned long id;

    #pragma db column(SigningScript::index_)
    uint32_t index;

    #pragma db column(SigningScript::label_)
    std::string label;

    #pragma db column(SigningScript::status_)
    SigningScript::status_t status;

    #pragma db column(SigningScript::txinscript_)
    bytes_t txinscript;

    #pragma db column(SigningScript::txoutscript_)
    bytes_t txoutscript;
};

#pragma db view \
    object(SigningScript) \
    object(Account: SigningScript::account_) \
    object(AccountBin: SigningScript::account_bin_)
struct ScriptCountView
{
    #pragma db column("count(" + SigningScript::id_ + ")")
    uint32_t count;

    #pragma db column("max(" + SigningScript::index_ + ")")
    unsigned long max_index;
};

const std::string EMPTY_STRING = "";

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(BlockHeader: Tx::blockheader_) \
    object(Account = sending_account: TxOut::sending_account_) \
    object(Account = receiving_account: TxOut::receiving_account_) \
    object(AccountBin: TxOut::account_bin_) \
    object(SigningScript: TxOut::signingscript_)
struct TxOutView
{
    TxOutView() : role_flags(TxOut::ROLE_NONE) { }
    TxOutView(const TxOutView& source, TxOut::role_t role)
    {
        *this = source;
        role_flags = role;
    }

    // must be called when queried - TODO: better style
    void updateRole(int flags = TxOut::ROLE_BOTH)
    {
        role_flags = TxOut::ROLE_NONE;
        if (sending_account_id)          { role_flags |= TxOut::ROLE_SENDER;   }
        if (receiving_account_id)        { role_flags |= TxOut::ROLE_RECEIVER; }
        role_flags &= flags;
    }


    const std::string& role_account() const
    {
        switch (role_flags)
        {
        case TxOut::ROLE_SENDER:
            return sending_account_name;

        case TxOut::ROLE_RECEIVER:
            return receiving_account_name;

        default:
            return EMPTY_STRING;
        }
    }

    const std::string& role_bin() const
    {
        if (role_flags == TxOut::ROLE_RECEIVER)
            return account_bin_name;
        else
            return EMPTY_STRING;
    }

    const std::string& role_label() const
    {
        switch (role_flags)
        {
        case TxOut::ROLE_SENDER:
            return sending_label;

        case TxOut::ROLE_RECEIVER:
            return receiving_label;

        default:
            return EMPTY_STRING;
        }
    }

    std::vector<TxOutView> getSplitRoles(TxOut::role_t first = TxOut::ROLE_RECEIVER, const std::string& account_name = "")
    {
        std::vector<TxOutView> split_views;

        if ((role_flags & TxOut::ROLE_RECEIVER) && (account_name.empty() || account_name == receiving_account_name))
        {
            split_views.push_back(TxOutView(*this, TxOut::ROLE_RECEIVER));
        }

        if ((role_flags & TxOut::ROLE_SENDER) && (account_name.empty() || account_name == sending_account_name))
        {
            split_views.push_back(TxOutView(*this, TxOut::ROLE_SENDER));
        }

        if (first == TxOut::ROLE_SENDER)
        {
            std::reverse(split_views.begin(), split_views.end());
        }

        return split_views;
    }

    #pragma db column(sending_account::id_)
    unsigned long sending_account_id;

    #pragma db column(sending_account::name_)
    std::string sending_account_name;

    #pragma db column(receiving_account::id_)
    unsigned long receiving_account_id;

    #pragma db column(receiving_account::name_)
    std::string receiving_account_name;

    #pragma db column(AccountBin::id_)
    unsigned long account_bin_id;

    #pragma db column(AccountBin::name_)
    std::string account_bin_name;

    #pragma db column(SigningScript::id_)
    unsigned long signingscript_id;

    #pragma db column(SigningScript::label_)
    std::string signingscript_label;

    #pragma db column(SigningScript::status_)
    SigningScript::status_t signingscript_status;

    #pragma db column(SigningScript::txinscript_)
    bytes_t signingscript_txinscript;

    #pragma db column(TxOut::script_)
    bytes_t script;

    #pragma db column(TxOut::value_)
    uint64_t value;

    #pragma db column(TxOut::status_)
    TxOut::status_t status;

    #pragma db transient
    int role_flags;

    #pragma db column(TxOut::sending_label_)
    std::string sending_label;

    #pragma db column(TxOut::receiving_label_)
    std::string receiving_label;

    #pragma db column(Tx::id_)
    unsigned long tx_id;

    #pragma db column(Tx::unsigned_hash_)
    bytes_t tx_unsigned_hash;

    #pragma db column(Tx::hash_)
    bytes_t tx_hash;

    #pragma db column(Tx::timestamp_)
    uint32_t tx_timestamp;

    #pragma db column(Tx::status_)
    Tx::status_t tx_status;

    #pragma db column(TxOut::txindex_)
    uint32_t tx_index;

    #pragma db column(Tx::have_fee_)
    bool have_fee;

    #pragma db column(Tx::fee_)
    uint64_t fee;

    #pragma db column(BlockHeader::height_)
    uint32_t height;
};

#pragma db view \
    object(TxOut) \
    object(Tx: TxOut::tx_) \
    object(BlockHeader: Tx::blockheader_) \
    object(Account: TxOut::receiving_account_) \
    object(AccountBin: TxOut::account_bin_) \
    object(SigningScript: TxOut::signingscript_)
struct BalanceView
{
    #pragma db column("sum(" + TxOut::value_ + ")")
    uint64_t balance;
};

#pragma db view \
    object(BlockHeader)
struct BestHeightView
{
    #pragma db column("max(" + BlockHeader::height_ + ")")
    uint32_t height;
};

#pragma db view \
    object(BlockHeader)
struct HorizonHeightView
{
    #pragma db column("min(" + BlockHeader::height_ + ")")
    uint32_t height;
};

#pragma db view \
    object(BlockHeader)
struct BlockCountView
{
    #pragma db column("count(" + BlockHeader::id_ + ")")
    unsigned long count;
};

#pragma db view \
    object(Account)
struct HorizonTimestampView
{
    #pragma db column("min(" + Account::time_created_ + ")")
    uint32_t timestamp;
};

#pragma db view \
    object(Tx) \
    table("MerkleBlock_hashes" = "t": "t.value = " + Tx::hash_) \
    object(MerkleBlock: "t.object_id = " + MerkleBlock::id_) \
    object(BlockHeader: MerkleBlock::blockheader_)
struct ConfirmedTxView
{
    #pragma db column(Tx::id_)
    unsigned long tx_id;

    #pragma db column(Tx::hash_)
    bytes_t tx_hash;

    #pragma db column(MerkleBlock::id_)
    unsigned long merkleblock_id;

    #pragma db column(BlockHeader::id_)
    unsigned long blockheader_id;

    #pragma db column(BlockHeader::hash_)
    bytes_t block_hash;

    #pragma db column(BlockHeader::height_)
    uint32_t block_height;
};

}

BOOST_CLASS_VERSION(CoinDB::TxIn, 1)
BOOST_CLASS_VERSION(CoinDB::TxOut, 1)
BOOST_CLASS_VERSION(CoinDB::Tx, 1)

BOOST_CLASS_VERSION(CoinDB::Keychain, 1)
BOOST_CLASS_VERSION(CoinDB::AccountBin, 1)
BOOST_CLASS_VERSION(CoinDB::Account, 1)

#endif // COINDB_SCHEMA_H
