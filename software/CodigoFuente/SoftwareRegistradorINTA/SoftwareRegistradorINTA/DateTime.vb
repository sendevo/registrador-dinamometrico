Public Class DateTime
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        'Se usan dos controles, uno para la fecha y otro para la hora, porque un solo control no
        'permite ajustar las dos variables
    End Sub

    Private Sub okBtn_Click(sender As Object, e As EventArgs) Handles okBtn.Click

        'Se asume que el año tiene 4 digitos siempre

        'Obtener mes
        Dim monthStr As String = DatePicker.Value.Month.ToString
        If DatePicker.Value.Month < 10 Then
            monthStr = "0" + monthStr
        End If

        'Obtener dia
        Dim dayStr As String = DatePicker.Value.Day.ToString
        If DatePicker.Value.Day < 10 Then
            dayStr = "0" + dayStr
        End If

        'Obtener hora
        Dim hourStr As String = TimePicker.Value.Hour.ToString
        If TimePicker.Value.Hour < 10 Then
            hourStr = "0" + hourStr
        End If

        'Obtener minutos
        Dim minuteStr As String = TimePicker.Value.Minute.ToString
        If TimePicker.Value.Minute < 10 Then
            minuteStr = "0" + minuteStr
        End If

        'Obtener hora
        Dim secondStr As String = TimePicker.Value.Second.ToString
        If TimePicker.Value.Second < 10 Then
            secondStr = "0" + secondStr
        End If

        'Generar comando para escribir en puerto serie
        Dim dateTimeStr As String = "e" + DatePicker.Value.Year.ToString +
            monthStr + dayStr + hourStr + minuteStr + secondStr + "\n"

        'Debug.WriteLine("Actualizando fecha y hora, comando = " + dateTimeStr)

        If MainWindow.SerialPort1.IsOpen Then
            MainWindow.SerialPort1.Write(dateTimeStr)
            MainWindow.StatusLabel.Text = "Fecha y hora ajustado correctamente."
            Close()
        Else
            MsgBox("El puerto serie no está conectado!")
        End If
    End Sub

    Private Sub cancelBtn_Click(sender As Object, e As EventArgs) Handles cancelBtn.Click
        Close()
    End Sub
End Class