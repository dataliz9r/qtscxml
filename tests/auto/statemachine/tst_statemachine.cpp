/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QObject>
#include <QXmlStreamReader>
#include <QtScxml/qscxmlparser.h>
#include <QtScxml/qscxmlstatemachine.h>

Q_DECLARE_METATYPE(QScxmlError);

enum { SpyWaitTime = 8000 };

class tst_StateMachine: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stateNames_data();
    void stateNames();
    void activeStateNames_data();
    void activeStateNames();
    void connections();
    void eventOccurred();

    void doneDotStateEvent();
};

void tst_StateMachine::stateNames_data()
{
    QTest::addColumn<QString>("scxmlFileName");
    QTest::addColumn<bool>("compressed");
    QTest::addColumn<QStringList>("expectedStates");

    QTest::newRow("stateNames-compressed") << QString(":/tst_statemachine/statenames.scxml")
                                      << true
                                      << (QStringList() << QString("a1") << QString("a2") << QString("final"));
    QTest::newRow("stateNames-notCompressed") << QString(":/tst_statemachine/statenames.scxml")
                                      << false
                                      << (QStringList() << QString("top") << QString("a") << QString("a1") <<  QString("a2") << QString("b") << QString("final"));
    QTest::newRow("stateNamesNested-compressed") << QString(":/tst_statemachine/statenamesnested.scxml")
                                      << true
                                      << (QStringList() << QString("a") << QString("b"));
    QTest::newRow("stateNamesNested-notCompressed") << QString(":/tst_statemachine/statenamesnested.scxml")
                                      << false
                                      << (QStringList() << QString("super_top") << QString("a") << QString("b"));

    QTest::newRow("ids1") << QString(":/tst_statemachine/ids1.scxml")
                          << false
                          << (QStringList() << QString("foo.bar") << QString("foo-bar")
                              << QString("foo_bar") << QString("_"));
}

void tst_StateMachine::stateNames()
{
    QFETCH(QString, scxmlFileName);
    QFETCH(bool, compressed);
    QFETCH(QStringList, expectedStates);

    QScopedPointer<QScxmlStateMachine> stateMachine(QScxmlStateMachine::fromFile(scxmlFileName));
    QVERIFY(!stateMachine.isNull());
    QCOMPARE(stateMachine->parseErrors().count(), 0);

    QCOMPARE(stateMachine->stateNames(compressed), expectedStates);
}

void tst_StateMachine::activeStateNames_data()
{
    QTest::addColumn<QString>("scxmlFileName");
    QTest::addColumn<bool>("compressed");
    QTest::addColumn<QStringList>("expectedStates");

    QTest::newRow("stateNames-compressed") << QString(":/tst_statemachine/statenames.scxml")
                                      << true
                                      << (QStringList() << QString("a1") << QString("final"));
    QTest::newRow("stateNames-notCompressed") << QString(":/tst_statemachine/statenames.scxml")
                                      << false
                                      << (QStringList() << QString("top") << QString("a") << QString("a1") << QString("b") << QString("final"));
    QTest::newRow("stateNamesNested-compressed") << QString(":/tst_statemachine/statenamesnested.scxml")
                                      << true
                                      << (QStringList() << QString("a") << QString("b"));
    QTest::newRow("stateNamesNested-notCompressed") << QString(":/tst_statemachine/statenamesnested.scxml")
                                      << false
                                      << (QStringList() << QString("super_top") << QString("a") << QString("b"));
}

void tst_StateMachine::activeStateNames()
{
    QFETCH(QString, scxmlFileName);
    QFETCH(bool, compressed);
    QFETCH(QStringList, expectedStates);

    QScopedPointer<QScxmlStateMachine> stateMachine(QScxmlStateMachine::fromFile(scxmlFileName));
    QVERIFY(!stateMachine.isNull());

    QSignalSpy stableStateSpy(stateMachine.data(), SIGNAL(reachedStableState()));

    stateMachine->start();

    stableStateSpy.wait(5000);

    QCOMPARE(stateMachine->activeStateNames(compressed), expectedStates);
}

class Receiver : public QObject {
    Q_OBJECT
public slots:
    void a(bool enabled)
    {
        aReached = aReached || enabled;
    }

    void b(bool enabled)
    {
        bReached = bReached || enabled;
    }

public:
    bool aReached = false;
    bool bReached = false;

};

void tst_StateMachine::connections()
{
    QScopedPointer<QScxmlStateMachine> stateMachine(
                QScxmlStateMachine::fromFile(QString(":/tst_statemachine/statenames.scxml")));
    QVERIFY(!stateMachine.isNull());
    Receiver receiver;

    bool a1Reached = false;
    bool finalReached = false;
    QMetaObject::Connection a = stateMachine->connectToState("a", &receiver, &Receiver::a);
    QVERIFY(a);
    QMetaObject::Connection b = stateMachine->connectToState("b", &receiver, SLOT(b(bool)));
    QVERIFY(b);
    QMetaObject::Connection a1 = stateMachine->connectToState("a1", &receiver,
                                                              [&a1Reached](bool enabled) {
        a1Reached = a1Reached || enabled;
    });
    QVERIFY(a1);
    QMetaObject::Connection final = stateMachine->connectToState("final",
                                                                 [&finalReached](bool enabled) {
        finalReached = finalReached || enabled;
    });
    QVERIFY(final);

    stateMachine->start();

    QTRY_VERIFY(a1Reached);
    QTRY_VERIFY(finalReached);
    QTRY_VERIFY(receiver.aReached);
    QTRY_VERIFY(receiver.bReached);

    QVERIFY(disconnect(a));
    QVERIFY(disconnect(b));
    QVERIFY(disconnect(a1));
    QVERIFY(disconnect(final));
}

void tst_StateMachine::eventOccurred()
{
    QScopedPointer<QScxmlStateMachine> stateMachine(QScxmlStateMachine::fromFile(QString(":/tst_statemachine/eventoccurred.scxml")));
    QVERIFY(!stateMachine.isNull());

    qRegisterMetaType<QScxmlEvent>();
    QSignalSpy finishedSpy(stateMachine.data(), SIGNAL(finished()));
    QSignalSpy eventOccurredSpy(stateMachine.data(), SIGNAL(eventOccurred(QScxmlEvent)));
    QSignalSpy externalEventOccurredSpy(stateMachine.data(), SIGNAL(externalEventOccurred(QScxmlEvent)));

    stateMachine->start();

    finishedSpy.wait(5000);

    auto event = [&eventOccurredSpy](int eventIndex) -> QScxmlEvent {
        return qvariant_cast<QScxmlEvent>(eventOccurredSpy.at(eventIndex).at(0));
    };

    QCOMPARE(eventOccurredSpy.count(), 4);
    QCOMPARE(event(0).name(), QLatin1String("internalEvent2"));
    QCOMPARE(event(0).eventType(), QScxmlEvent::ExternalEvent);
    QCOMPARE(event(1).name(), QLatin1String("externalEvent"));
    QCOMPARE(event(1).eventType(), QScxmlEvent::ExternalEvent);
    QCOMPARE(event(2).name(), QLatin1String("timeout"));
    QCOMPARE(event(2).eventType(), QScxmlEvent::ExternalEvent);
    QCOMPARE(event(3).name(), QLatin1String("done.state.top"));
    QCOMPARE(event(3).eventType(), QScxmlEvent::ExternalEvent);

    QCOMPARE(externalEventOccurredSpy.count(), 1);
    QCOMPARE(qvariant_cast<QScxmlEvent>(externalEventOccurredSpy.at(0).at(0)).name(), QLatin1String("externalEvent"));
}

void tst_StateMachine::doneDotStateEvent()
{
    QScopedPointer<QScxmlStateMachine> stateMachine(QScxmlStateMachine::fromFile(QString(":/tst_statemachine/stateDotDoneEvent.scxml")));
    QVERIFY(!stateMachine.isNull());

    QSignalSpy finishedSpy(stateMachine.data(), SIGNAL(finished()));

    stateMachine->start();
    finishedSpy.wait(5000);
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(stateMachine->activeStateNames(true).size(), 1);
    qDebug() << stateMachine->activeStateNames(true);
    QVERIFY(stateMachine->activeStateNames(true).contains(QLatin1String("success")));
}


QTEST_MAIN(tst_StateMachine)

#include "tst_statemachine.moc"


