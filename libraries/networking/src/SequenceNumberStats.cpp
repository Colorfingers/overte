//
//  SequenceNumberStats.cpp
//  libraries/networking/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SequenceNumberStats.h"

#include <limits>

SequenceNumberStats::SequenceNumberStats(int statsHistoryLength)
    : _received(0),
    _lastReceivedSequence(0),
    _missingSet(),
    _stats(),
    _lastSenderUUID(),
    _statsHistory(statsHistoryLength)
{
}

void SequenceNumberStats::reset() {
    _received = 0;
    _missingSet.clear();
    _stats = PacketStreamStats();
    _lastSenderUUID = QUuid();
    _statsHistory.clear();
}

static const int UINT16_RANGE = std::numeric_limits<uint16_t>::max() + 1;

SequenceNumberStats::ArrivalInfo SequenceNumberStats::sequenceNumberReceived(quint16 incoming, QUuid senderUUID, const bool wantExtraDebugging) {

    SequenceNumberStats::ArrivalInfo arrivalInfo;

    // if the sender node has changed, reset all stats
    if (senderUUID != _lastSenderUUID) {
        if (_received > 0) {
            qDebug() << "sequence number stats was reset due to new sender node";
            qDebug() << "previous:" << _lastSenderUUID << "current:" << senderUUID;
            reset();
        }
        _lastSenderUUID = senderUUID;
    }

    // determine our expected sequence number... handle rollover appropriately
    quint16 expected = _received > 0 ? _lastReceivedSequence + (quint16)1 : incoming;

    _received++;

    if (incoming == expected) { // on time
        arrivalInfo._status = OnTime;
        _lastReceivedSequence = incoming;
        _stats._expectedReceived++;
    } else { // out of order

        if (wantExtraDebugging) {
            qDebug() << "out of order... got:" << incoming << "expected:" << expected;
        }

        int incomingInt = (int)incoming;
        int expectedInt = (int)expected;

        // check if the gap between incoming and expected is reasonable, taking possible rollover into consideration
        int absGap = std::abs(incomingInt - expectedInt);
        if (absGap >= UINT16_RANGE - MAX_REASONABLE_SEQUENCE_GAP) {
            // rollover likely occurred between incoming and expected.
            // correct the larger of the two so that it's within [-UINT16_RANGE, -1] while the other remains within [0, UINT16_RANGE-1]
            if (incomingInt > expectedInt) {
                incomingInt -= UINT16_RANGE;
            } else {
                expectedInt -= UINT16_RANGE;
            }
        } else if (absGap > MAX_REASONABLE_SEQUENCE_GAP) {
            arrivalInfo._status = Unreasonable;

            // ignore packet if gap is unreasonable
            qDebug() << "ignoring unreasonable sequence number:" << incoming
                << "previous:" << _lastReceivedSequence;
            _stats._unreasonable++;

            return arrivalInfo;
        }

        // now that rollover has been corrected for (if it occurred), incomingInt and expectedInt can be
        // compared to each other directly, though one of them might be negative

        arrivalInfo._seqDiffFromExpected = incomingInt - expectedInt;

        if (incomingInt > expectedInt) { // early
            arrivalInfo._status = Early;

            if (wantExtraDebugging) {
                qDebug() << "this packet is earlier than expected...";
                qDebug() << ">>>>>>>> missing gap=" << (incomingInt - expectedInt);
            }
            int skipped = incomingInt - expectedInt;
            _stats._early++;
            _stats._lost += skipped;
            _stats._expectedReceived += (skipped + 1);
            _lastReceivedSequence = incoming;

            // add all sequence numbers that were skipped to the missing sequence numbers list
            for (int missingInt = expectedInt; missingInt < incomingInt; missingInt++) {
                _missingSet.insert((quint16)(missingInt < 0 ? missingInt + UINT16_RANGE : missingInt));
            }
            
            // prune missing sequence list if it gets too big; sequence numbers that are older than MAX_REASONABLE_SEQUENCE_GAP
            // will be removed.
            if (_missingSet.size() > MAX_REASONABLE_SEQUENCE_GAP) {
                pruneMissingSet(wantExtraDebugging);
            }
        } else { // late
            if (wantExtraDebugging) {
                qDebug() << "this packet is later than expected...";
            }

            _stats._late++;

            // do not update _lastReceived; it shouldn't become smaller

            // remove this from missing sequence number if it's in there
            if (_missingSet.remove(incoming)) {
                arrivalInfo._status = Late;

                if (wantExtraDebugging) {
                    qDebug() << "found it in _missingSet";
                }
                _stats._lost--;
                _stats._recovered++;
            } else {
                arrivalInfo._status = Duplicate;

                if (wantExtraDebugging) {
                    qDebug() << "sequence:" << incoming << "was NOT found in _missingSet and is probably a duplicate";
                }
                _stats._duplicate++;
            }
        }
    }
    return arrivalInfo;
}

void SequenceNumberStats::pruneMissingSet(const bool wantExtraDebugging) {
    if (wantExtraDebugging) {
        qDebug() << "pruning _missingSet! size:" << _missingSet.size();
    }

    // some older sequence numbers may be from before a rollover point; this must be handled.
    // some sequence numbers in this list may be larger than _incomingLastSequence, indicating that they were received
    // before the most recent rollover.
    int cutoff = (int)_lastReceivedSequence - MAX_REASONABLE_SEQUENCE_GAP;
    if (cutoff >= 0) {
        quint16 nonRolloverCutoff = (quint16)cutoff;
        QSet<quint16>::iterator i = _missingSet.begin();
        while (i != _missingSet.end()) {
            quint16 missing = *i;
            if (wantExtraDebugging) {
                qDebug() << "checking item:" << missing << "is it in need of pruning?";
                qDebug() << "old age cutoff:" << nonRolloverCutoff;
            }

            if (missing > _lastReceivedSequence || missing < nonRolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qDebug() << "pruning really old missing sequence:" << missing;
                }
            } else {
                i++;
            }
        }
    } else {
        quint16 rolloverCutoff = (quint16)(cutoff + UINT16_RANGE);
        QSet<quint16>::iterator i = _missingSet.begin();
        while (i != _missingSet.end()) {
            quint16 missing = *i;
            if (wantExtraDebugging) {
                qDebug() << "checking item:" << missing << "is it in need of pruning?";
                qDebug() << "old age cutoff:" << rolloverCutoff;
            }

            if (missing > _lastReceivedSequence && missing < rolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qDebug() << "pruning really old missing sequence:" << missing;
                }
            } else {
                i++;
            }
        }
    }
}

PacketStreamStats SequenceNumberStats::getStatsForHistoryWindow() const {

    const PacketStreamStats* newestStats = _statsHistory.getNewestEntry();
    const PacketStreamStats* oldestStats = _statsHistory.get(_statsHistory.getNumEntries() - 1);
    
    // this catches cases where history is length 1 or 0 (both are NULL in case of 0)
    if (newestStats == oldestStats) {
        return PacketStreamStats();
    }

    // calculate difference between newest stats and oldest stats to get window stats
    PacketStreamStats windowStats;
    windowStats._expectedReceived = newestStats->_expectedReceived - oldestStats->_expectedReceived;
    windowStats._unreasonable = newestStats->_unreasonable - oldestStats->_unreasonable;
    windowStats._early = newestStats->_early - oldestStats->_early;
    windowStats._late = newestStats->_late - oldestStats->_late;
    windowStats._lost = newestStats->_lost - oldestStats->_lost;
    windowStats._recovered = newestStats->_recovered - oldestStats->_recovered;
    windowStats._duplicate = newestStats->_duplicate - oldestStats->_duplicate;

    return windowStats;
}
