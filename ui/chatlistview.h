#pragma once
#include <QListView>

class ChatListView : public QListView
{
    Q_OBJECT

public:
    explicit ChatListView(QWidget *parent = nullptr);
};