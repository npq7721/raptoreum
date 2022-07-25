// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/raptoreum-config.h>
#endif

#include <qt/walletmodelfuturestransaction.h>
#include <rpc/specialtx_utilities.h>
#include <timedata.h>

#include <wallet/wallet.h>
#include <key_io.h>

WalletModelFuturesTransaction::WalletModelFuturesTransaction(const QList<SendFuturesRecipient> &_recipients) :
    recipients(_recipients),
    fee(0)
{
    //walletTransaction = new CFutureTx();
   // walletTransaction = new CWalletTx();
}

CTransactionRef& WalletModelFuturesTransaction::getWtx()
{
    return wtx;
}


QList<SendFuturesRecipient> WalletModelFuturesTransaction::getRecipients() const
{
    return recipients;
}

unsigned int WalletModelFuturesTransaction::getTransactionSize() const
{
    return wtx != nullptr ? ::GetSerializeSize(*wtx, SER_NETWORK, PROTOCOL_VERSION) : 0;
}

CAmount WalletModelFuturesTransaction::getTransactionFee() const
{
    return fee;
}

void WalletModelFuturesTransaction::setTransactionFee(const CAmount& newFee)
{
    fee = newFee;
}

void WalletModelFuturesTransaction::assignFuturePayload() {
	for (QList<SendFuturesRecipient>::iterator it = recipients.begin(); it != recipients.end(); ++it)
	{
		SendFuturesRecipient& rcp = (*it);
		// normal recipient (no payment request)
#ifdef ENABLE_BIP70
		if (!rcp.paymentRequest.IsInitialized())
		{
			CFutureTx ftx;
			ftx.nVersion = CFutureTx::CURRENT_VERSION;
			ftx.lockOutputIndex = 0;
			ftx.updatableByDestination = false;
			const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
      for (int j = 0; j < details.outputs_size(); j++) {
          const payments::Output& out = details.outputs(j);
          if (out.amount() <= 0) continue;
          const unsigned char* scriptStr = (const unsigned char*)out.script().data();
          CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
          for (const auto& txout : wtx.get()->vout) {
              if (txout.scriptPubKey == scriptPubKey) {
                  rcp.amount = txout.nValue;
                  ftx.lockTime = rcp.locktime - GetAdjustedTime();
                  ftx.maturity = rcp.maturity;
                  break;
              }
          }
				  ftx.lockOutputIndex++;
			}
		}
#endif
	}
}

void WalletModelFuturesTransaction::reassignAmounts()
{
    // For each recipient look for a matching CTxOut in walletTransaction and reassign amounts

    for (QList<SendFuturesRecipient>::iterator it = recipients.begin(); it != recipients.end(); ++it)
    {
        SendFuturesRecipient& rcp = (*it);

#ifdef ENABLE_BIP70
        if (rcp.paymentRequest.IsInitialized())
        {
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int j = 0; j < details.outputs_size(); j++)
            {
                const payments::Output& out = details.outputs(j);
                if (out.amount() <= 0) continue;
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                for (const auto& txout : wtx.get()->vout) {
                    if (txout.scriptPubKey == scriptPubKey) {
                        subtotal += txout.nValue;
                        break;
                    }
                }
            }
            rcp.amount = subtotal;
        }
        else // normal recipient (no payment request)
#endif
        {
            CFutureTx ftx;
            for (const auto& txout : wtx.get()->vout) {
                CScript scriptPubKey = GetScriptForDestination(DecodeDestination(rcp.address.toStdString()));
                if (txout.scriptPubKey == scriptPubKey) {
                    rcp.amount = txout.nValue;
                    break;
                }
            }
        }
    }
}

CAmount WalletModelFuturesTransaction::getTotalTransactionAmount() const
{
    CAmount totalTransactionAmount = 0;
    for (const SendFuturesRecipient &rcp : recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}
