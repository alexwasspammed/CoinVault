///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountmodel.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTMODEL_H
#define COINVAULT_ACCOUNTMODEL_H

#include <QStandardItemModel>

#include <QPair>

#include <CoinDB/Vault.h>

#include <CoinQ/CoinQ_typedefs.h>

class TaggedOutput
{
public:
    TaggedOutput(const bytes_t& script, uint64_t value, const std::string& tag)
        : script_(script), value_(value), tag_(tag) { }

    const bytes_t& script() const { return script_; }
    uint64_t value() const { return value_; }
    const std::string& tag() const { return tag_; }
    bool isTagged() const { return !tag_.empty(); }

private:
    bytes_t script_;
    uint64_t value_;
    std::string tag_;
};

class AccountModel : public QStandardItemModel
{
    Q_OBJECT

public:
    AccountModel();
    ~AccountModel() { if (vault) delete vault; }

    void update();

    // Vault operations
    void create(const QString& fileName);
    void load(const QString& fileName);
    void close();
    bool isOpen() const { return (vault != NULL); }
    Coin::BloomFilter getBloomFilter(double falsePositiveRate, uint32_t nTweak, uint32_t nFlags = 0) const;

    // Key Chain operations
    void newKeychain(const QString& name, const secure_bytes_t& entropy);

    // Account operations
    void newAccount(const QString& name, unsigned int minsigs, const QList<QString>& keychainNames);
    bool accountExists(const QString& name) const;
    void exportAccount(const QString& name, const QString& filePath) const;
    void importAccount(const QString& name, const QString& filePath);
    void deleteAccount(const QString& name);
    QPair<QString, bytes_t> issueNewScript(const QString& accountName, const QString& label); // returns qMakePair<address, script>
    uint32_t getMaxFirstBlockTimestamp() const; // the timestamp for latest acceptable first block

    // Transaction operations
    bool insertRawTx(const bytes_t& rawTx);
    std::shared_ptr<CoinDB::Tx> insertTx(std::shared_ptr<CoinDB::Tx> tx, bool sign = false);
    std::shared_ptr<CoinDB::Tx> createTx(const QString& accountName, std::vector<std::shared_ptr<CoinDB::TxOut>> txouts, uint64_t fee);
    bytes_t createRawTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    // TODO: not sure I'm too happy about exposing Coin:Vault::Tx
    std::shared_ptr<CoinDB::Tx> insertTx(const Coin::Transaction& coinTx, CoinDB::Tx::status_t status = CoinDB::Tx::PROPAGATED, bool sign = false);
    Coin::Transaction createTx(const QString& accountName, const std::vector<TaggedOutput>& outputs, uint64_t fee);
    bytes_t signRawTx(const bytes_t& rawTx);

    // Block operations
    std::vector<bytes_t> getLocatorHashes() const;
    bool insertBlock(const ChainBlock& block);
    bool insertMerkleBlock(const ChainMerkleBlock& merkleBlock);
    bool deleteMerkleBlock(const bytes_t& hash);

    CoinDB::Vault* getVault() const { return vault; }
    int getNumAccounts() const { return numAccounts; }

    // Overridden methods
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex& index) const;

signals:
    void updated(const QStringList& accountNames);
    void newTx(const bytes_t& hash);
    void newBlock(const bytes_t& hash, int height);
    void updateSyncHeight(int height);

    void error(const QString& message);

private:
    unsigned char base58_versions[2];

    CoinDB::Vault* vault;
    int numAccounts;
};

#endif // COINVAULT_ACCOUNTMODEL_H
