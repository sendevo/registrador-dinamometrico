Public Class Monitor

    Private Sub Form2_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        CheckForIllegalCrossThreadCalls = False 'Esto es para modificar el formulario desde otro Sub y que no de error
    End Sub

    Public Sub updateBars(b1 As Integer, b2 As Integer, b3 As Integer, b4 As Integer)
        ProgressBar1.Value = b1
        ProgressBar2.Value = b2
        ProgressBar3.Value = b3
        ProgressBar4.Value = b4
        Label1.Text = "Celda 1 = " + b1.ToString() + "%"
        Label2.Text = "Celda 2 = " + b2.ToString() + "%"
        Label3.Text = "Celda 3 = " + b3.ToString() + "%"
        Label4.Text = "Celda 4 = " + b4.ToString() + "%"
    End Sub

    Private Sub Form2_Closing(sender As Object, e As System.ComponentModel.CancelEventArgs) Handles MyBase.Closing
        MainWindow.Timer1.Stop() 'Al cerrar esta ventana, detener el Timer1 para que no siga pidiendo valores
        MainWindow.StatusLabel.Text = ""
    End Sub
End Class