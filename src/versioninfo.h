///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// versioninfo.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef VAULT_VERSIONINFO_H
#define VAULT_VERSIONINFO_H

#include <QString>

const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 30;

const QString VERSIONTEXT("Version 0.1.3 beta");

const QString& getCommitHash();
const QString& getShortCommitHash();

#endif //  VAULT_VERSIONINFO_H
