#include "exodus/test/utils_tx.h"

#include "exodus/exodus.h"
#include "exodus/script.h"
#include "exodus/tx.h"
#include "exodus/encoding.h"
#include "exodus/createpayload.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"

#include <stdint.h>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace exodus;

BOOST_FIXTURE_TEST_SUITE(exodus_sender_firstin_tests, BasicTestingSetup)

/** Creates a dummy class C transaction with the given inputs. */
static CTransaction TxClassC(const std::vector<CTxOut>& txInputs)
{
    CMutableTransaction mutableTx;

    // Inputs:
    for (std::vector<CTxOut>::const_iterator it = txInputs.begin(); it != txInputs.end(); ++it)
    {
        const CTxOut& txOut = *it;

        // Create transaction for input:
        CMutableTransaction inputTx;
        unsigned int nOut = 0;
        inputTx.vout.push_back(txOut);
        CTransaction tx(inputTx);

        // Populate transaction cache:
        CCoinsModifier coins = view.ModifyCoins(tx.GetHash());

        if (nOut >= coins->vout.size()) {
            coins->vout.resize(nOut+1);
        }
        coins->vout[nOut].scriptPubKey = txOut.scriptPubKey;
        coins->vout[nOut].nValue = txOut.nValue;

        // Add input:
        CTxIn txIn(tx.GetHash(), nOut);
        mutableTx.vin.push_back(txIn);
    }

    // Outputs:
    std::vector<std::pair<CScript, int64_t> > txOutputs;
    std::vector<unsigned char> vchPayload = CreatePayload_SimpleSend(1, 1000);
    BOOST_CHECK(Exodus_Encode_ClassC(vchPayload, txOutputs));

    for (std::vector<std::pair<CScript, int64_t> >::const_iterator it = txOutputs.begin(); it != txOutputs.end(); ++it)
    {
        CTxOut txOut(it->second, it->first);
        mutableTx.vout.push_back(txOut);
    }

    return CTransaction(mutableTx);
}

/** Helper to create a CTxOut object. */
static CTxOut createTxOut(int64_t amount, const std::string& dest)
{
    return CTxOut(amount, GetScriptForDestination(CBitcoinAddress(dest).Get()));
}

/** Extracts the "first" sender. */
static bool GetFirstSender(const std::vector<CTxOut>& txInputs, std::string& strSender)
{
    int nBlock = std::numeric_limits<int>::max();

    CMPTransaction metaTx;
    CTransaction dummyTx = TxClassC(txInputs);

    if (ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0) {
        strSender = metaTx.getSender();
        return true;
    }

    return false;
}

BOOST_AUTO_TEST_CASE(first_vin_is_sender)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(100, "TYB7nvkkRCpqHNSyLuWSYqTumrmS3riDj2")); // Winner
    vouts.push_back(createTxOut(999, "TLiTEjj7ZgkU4u9Mvv8L43ehbx3JvyZACa"));
    vouts.push_back(createTxOut(200, "TEfpYzPybo9dDdL7SwuA7ii4g41TjH6GLd"));

    std::string strExpected("TYB7nvkkRCpqHNSyLuWSYqTumrmS3riDj2");

    std::string strSender;
    BOOST_CHECK(GetFirstSender(vouts, strSender));
    BOOST_CHECK_EQUAL(strExpected, strSender);
}

BOOST_AUTO_TEST_CASE(less_input_restrictions)
{
    std::vector<CTxOut> vouts;
    vouts.push_back(createTxOut(555, "TYB7nvkkRCpqHNSyLuWSYqTumrmS3riDj2")); // Winner
    vouts.push_back(PayToPubKey_Unrelated());
    vouts.push_back(PayToBareMultisig_1of3());
    vouts.push_back(NonStandardOutput());

    std::string strExpected("TYB7nvkkRCpqHNSyLuWSYqTumrmS3riDj2");

    std::string strSender;
    BOOST_CHECK(GetFirstSender(vouts, strSender));
    BOOST_CHECK_EQUAL(strExpected, strSender);
}

BOOST_AUTO_TEST_CASE(invalid_inputs)
{
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToPubKey_Unrelated());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(PayToBareMultisig_1of3());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
    }
    {
        std::vector<CTxOut> vouts;
        vouts.push_back(NonStandardOutput());
        std::string strSender;
        BOOST_CHECK(!GetFirstSender(vouts, strSender));
    }
}


BOOST_AUTO_TEST_SUITE_END()
