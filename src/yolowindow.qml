import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: "Add Button Example"

    Button {
        id: myButton
        text: "Click me"
        anchors.centerIn: parent
        onClicked: {
            console.log("Button clicked")
        }
    }
}