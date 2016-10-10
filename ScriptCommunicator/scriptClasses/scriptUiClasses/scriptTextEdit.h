/***************************************************************************
**                                                                        **
**  ScriptCommunicator, is a tool for sending/receiving data with several **
**  interfaces.                                                           **
**  Copyright (C) 2014 Stefan Zieker                                      **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Stefan Zieker                                        **
**  Website/Contact: http://sourceforge.net/projects/scriptcommunicator/  **
****************************************************************************/

#ifndef SCRIPTTEXTEDIT_H
#define SCRIPTTEXTEDIT_H

#include <QObject>
#include <QTextEdit>
#include <QVector>
#include "mainwindow.h"
#include "scriptWidget.h"



///This wrapper class is used to access a QTextEdit object (located in a script gui/ui-file) from a script.
class ScriptTextEdit : public ScriptWidget
{
    Q_OBJECT
public:
    explicit ScriptTextEdit(QTextEdit* textEdit, ScriptThread *scriptThread, ScriptWindow* scriptWindow ):
        ScriptWidget(textEdit, scriptThread, scriptThread->getScriptWindow()), m_textEdit(textEdit), m_maxChars(100000),
        m_lockScrolling(false), m_storedOperations(), m_bytesInStoredOperations(0), m_storedOperationTimer(), m_updateRate(200)
    {

        //connect the necessary signals with the wrapper slots (in this slots the
        //events of the wrapper class are generated, the script can connect to this
        //wrapper events)
        Qt::ConnectionType directConnectionType = scriptThread->runsInDebugger() ? Qt::DirectConnection : Qt::BlockingQueuedConnection;

        connect(m_textEdit, SIGNAL(textChanged()), this, SIGNAL(textChangedSignal()));
        connect(this, SIGNAL(limtCharsInTextEditSignal(QTextEdit*,int)),
                scriptWindow, SLOT(limtCharsInTextEditSlot(QTextEdit*,int)), directConnectionType);
        connect(this, SIGNAL(setPlainTextSignal(QString)),m_textEdit, SLOT(setPlainText(QString)), directConnectionType);
        connect(this, SIGNAL(setTextSignal(QString)), m_textEdit, SLOT(setText(QString)), directConnectionType);
        connect(this, SIGNAL(moveTextPositionToEndSignal(QTextEdit*)),scriptWindow, SLOT(moveTextPositionToEndSlot(QTextEdit*)), directConnectionType);
        connect(this, SIGNAL(verticalScrollBarSetValueSignal(int)),m_textEdit->verticalScrollBar(), SLOT(setValue(int)), directConnectionType);
        connect(this, SIGNAL(setFontPointSizeSignal(qreal)),m_textEdit, SLOT(setFontPointSize(qreal)), directConnectionType);
        connect(this, SIGNAL(setFontFamilySignal(QString)),m_textEdit, SLOT(setFontFamily(QString)), directConnectionType);
        connect(&m_storedOperationTimer, SIGNAL(timeout()),this, SLOT(storedTimerElapsedSlot()), Qt::QueuedConnection);


        connect(this, SIGNAL(processStoredOperationsSignal(QTextEdit*,bool,quint32,QVector<ScriptTextEditStoredOperations_t>*)), scriptThread->getScriptWindow(),
                SLOT(processStoredOperationsSlot(QTextEdit*,bool,quint32,QVector<ScriptTextEditStoredOperations_t>*)), directConnectionType);

    }

    ///Returns a semicolon separated list with all public functions, signals and properties.
    virtual QString getPublicScriptElements(void)
    {
        return ScriptWidget::getPublicScriptElements() +
                ";int verticalScrollBarValue(void);void verticalScrollBarSetValue(int value);"
                "QString toPlainText(void);QString toHtml(void);"
                "void setMaxChars(int maxChars);QString replaceNonHtmlChars(QString text);"
                "void moveTextPositionToEnd(void);void setFontPointSize(qreal fontSize);"
                "void setFontFamily(QString fontFamily);void clear(void);"
                "void insertPlainText(QString text, bool atTheEnd=true);void insertHtml(QString htmlString, bool atTheEnd=true);"
                "void append(QString text);void setPlainText(QString text);"
                "void setText(QString text);void lockScrolling(bool lock);void textChangedSignal(void);"
                "void setUpdateRate(int updateRate)";
    }

    ///Returns the vertical scroll bar value.
    Q_INVOKABLE int verticalScrollBarValue(void){return m_textEdit->verticalScrollBar()->value();}

    ///Sets the vertical scroll bar value.
    Q_INVOKABLE void verticalScrollBarSetValue(int value){ emit verticalScrollBarSetValueSignal(value);}

    ///Returns the content of the text edit as plain text.
    Q_INVOKABLE QString toPlainText(void){if(!m_storedOperations.isEmpty()){storedTimerElapsedSlot();}return m_textEdit->toPlainText();}

    ///Returns the content of the text edit as html.
    Q_INVOKABLE QString toHtml(void){if(!m_storedOperations.isEmpty()){storedTimerElapsedSlot();}return m_textEdit->toHtml();}

    ///Sets the max. number of chars in the text edit.
    Q_INVOKABLE void setMaxChars(int maxChars){ m_maxChars = maxChars;}

    ///Replaces the characters '\n',' ', '<' and '>' to their html representation.
    Q_INVOKABLE QString replaceNonHtmlChars(QString text)
    {
        text.replace("<", "&lt;");
        text.replace(">", "&gt;");
        text.replace("\n", "<br>");
        text.replace(" ", "&nbsp;");
        return text;
    }
    ///Moves the curser to the end of the text edit.
    Q_INVOKABLE void moveTextPositionToEnd(void){emit moveTextPositionToEndSignal(m_textEdit);}

    ///Sets the update rate of the script text edit.
    Q_INVOKABLE void setUpdateRate(int updateRate){m_updateRate = updateRate;}

public Q_SLOTS:

    ///This slot function sets font size.
    void setFontPointSize(qreal fontSize){emit setFontPointSizeSignal(fontSize);}

    ///This slot function sets font family.
    void setFontFamily(QString fontFamily){emit setFontFamilySignal(fontFamily);}

    ///This slot function clears the text edit.
    void clear(void){QString tmp;addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_CLEAR, tmp, false);}

    ///This slot function inserts plain text into the text edit.
    void insertPlainText(QString text, bool atTheEnd=true){addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_INSERT_PLAIN_TEXT, text, atTheEnd);}

    ///This slot function inserts HTML text into the text edit.
    void insertHtml(QString htmlString, bool atTheEnd=true){addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_INSERT_HTML, htmlString, atTheEnd);}

    ///This slot function appends text at the end of text edit (includes a new line) and moves the cursor to the end of the text.
    void append(QString text){addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_APPEND, text, true);}

    ///This slot function sets the text of the text edit (plain text).
    void setPlainText(QString text){addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_SET_PLAIN_TEXT, text, true);}

    ///This slot function sets the text of the text edit.
    void setText(QString text){addStoredOperation(SCRIPT_TEXT_EDIT_OPERATION_SET_TEXT, text, true);}

	///Locks or unlocks the scrolling of the vertical scroll bar.
    void lockScrolling(bool lock){m_lockScrolling = lock;}

Q_SIGNALS:
    ///This signal is emitted if the text of the text edit has been changed.
    ///Scripts can connect a function to this signal.
    void textChangedSignal(void);

    ///This signal is emitted if the number of chars in text has to be limited.
    ///If there are more then maxChars characters in the text edit then the first characters will be removed.
    ///This signal is private and must not be used inside a script.
    void limtCharsInTextEditSignal(QTextEdit* textEdit, const int maxChars);

    ///This signal is emitted if the setPlainText function is called.
    ///This signal is private and must not be used inside a script.
    void setPlainTextSignal(const QString &text);

    ///This signal is emitted if the setText function is called.
    ///This signal is private and must not be used inside a script.
    void setTextSignal(const QString &text);

    ///This signal is emitted if the moveTextPositionToEnd function is called.
    ///This signal is private and must not be used inside a script.
    void moveTextPositionToEndSignal(QTextEdit* textEdit);

    ///This signal is emitted if the verticalScrollBarSetValue function is called.
    ///This signal is private and must not be used inside a script.
    void verticalScrollBarSetValueSignal(int);

    ///This signal is emitted if the setFontPointSize function is called.
    ///This signal is private and must not be used inside a script.
    void setFontPointSizeSignal(qreal);

    ///This signal is emitted if the setFontFamily function is called.
    ///This signal is private and must not be used inside a script.
    void setFontFamilySignal(QString);

    ///This signal is emitted in storedTimerElapsedSlot
    ///This signal is private and must not be used inside a script.
    void processStoredOperationsSignal(QTextEdit* textEdit, bool isLocked, quint32 maxChars, QVector<ScriptTextEditStoredOperations_t>* m_storedOperations);

private Q_SLOTS:

    ///Process the stored operations.
    void storedTimerElapsedSlot(void)
    {
        m_storedOperationTimer.stop();
        emit processStoredOperationsSignal(m_textEdit, m_lockScrolling, m_maxChars, &m_storedOperations);
        m_bytesInStoredOperations = 0;
        m_storedOperations.clear();
    }

private:

    ///Adds a stored operation to m_storedOperations.
    void inline addStoredOperation(ScriptTextEditOperation_t operation, QString& data, bool atTheEnd)
    {

        if((operation  == SCRIPT_TEXT_EDIT_OPERATION_CLEAR) || (operation  == SCRIPT_TEXT_EDIT_OPERATION_SET_PLAIN_TEXT) ||
           (operation  == SCRIPT_TEXT_EDIT_OPERATION_SET_TEXT))
        {
            m_bytesInStoredOperations = 0;
            m_storedOperations.clear();
        }

        if(!m_storedOperations.isEmpty() && (operation != SCRIPT_TEXT_EDIT_OPERATION_APPEND) &&
           (m_storedOperations.last().operation == operation) && (m_storedOperations.last().atTheEnd == atTheEnd))
        {
            //Append the data from the current operation at the data from last operation.
            m_storedOperations.last().data += data;
        }
        else
        {
            m_storedOperations.append({operation, data, atTheEnd});
        }
        m_bytesInStoredOperations += data.length();

        if(m_bytesInStoredOperations > m_maxChars)
        {//m_bytesInStoredOperations contains too much data.

            //Limit the size of m_storedOperations.
            while(m_bytesInStoredOperations > m_maxChars)
            {
                int diff = m_bytesInStoredOperations - m_maxChars;
                if(diff >= m_storedOperations.at(0).data.length())
                {
                    m_bytesInStoredOperations -= m_storedOperations.first().data.length();
                    m_storedOperations.removeFirst();
                }
                else
                {
                    m_storedOperations.first().data.remove(0, m_bytesInStoredOperations - m_maxChars);
                    m_bytesInStoredOperations -= m_bytesInStoredOperations - m_maxChars;
                }
            }
            //m_storedOperations contains more then m_maxChars, therefore all data in the text edit
            //can be deleted.
            m_storedOperations.push_front({SCRIPT_TEXT_EDIT_OPERATION_CLEAR, QString(""), true});
        }


        ///Start the timer if it is stopped.
        if(!m_storedOperationTimer.isActive())
        {
            m_storedOperationTimer.start(m_updateRate);
        }
    }

    ///The wrapped text edit.
    QTextEdit* m_textEdit;

    ///The maximum number of chars in the text edit.
    int m_maxChars;

    ///True if the scrolling is locked.
    bool m_lockScrolling;

    ///Contains all stored operations.
    QVector<ScriptTextEditStoredOperations_t> m_storedOperations;

    ///The number of bytes in m_storedOperations.
    int m_bytesInStoredOperations;

    ///Timer for processing the stored operations.
    QTimer m_storedOperationTimer;

    ///The update rate of the text edit.
    int m_updateRate;

};

#endif // SCRIPTTEXTEDIT_H
