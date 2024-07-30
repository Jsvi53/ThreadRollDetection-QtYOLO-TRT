#ifndef QSWITCHBUTTON_H
#define QSWITCHBUTTON_H

#include <QWidget>
#include <QTimer>

class SwitchButton : public QWidget
{
    Q_OBJECT

public:
    explicit SwitchButton(QWidget *parent = nullptr);
    ~SwitchButton();

    // return button state: true or false
    bool isToggled() const;

    // set button state
    void setToggled(bool checked);

    // set button background color
    void setBackgroundColor(QColor color);

    // set selected color
    void setCheckedColor(QColor color);

    // set unselected color
    void setDisabledColor(QColor color);


protected:
    // draw button
    void paintEvent(QPaintEvent *event) override;

    // mouse press event
    void mousePressEvent(QMouseEvent *event) override;

    // mouse release event, changing button state and emitting toggled() signal
    void mouseReleaseEvent(QMouseEvent *event) override;

    // change size event
    void resizeEvent(QResizeEvent *event) override;

    // default size
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // toggled signal
    void toggled(bool checked);

private slots:
    // change button state
    void onTimeout();

private:
    bool m_bChecked; // button state
    QColor m_backgroundColor; // button background color
    QColor m_checkedColor; // selected color
    QColor m_disabledColor; // unselected color
    QColor m_thumbColor; // thumb color
    qreal m_radius; // button radius
    qreal m_nX; // thumb x position
    qreal m_nY; // thumb y position
    qint16 m_nHeight; // thumb height
    qint16 m_nMargin; // thumb margin
    QTimer m_timer; // timer
};

#endif