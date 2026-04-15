using System;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Data.Pdf;
public static class PdfHelper {
    public static int GetPageCount(string path) {
        StorageFile file = Task.Run(() => StorageFile.GetFileFromPathAsync(path).AsTask()).Result;
        PdfDocument doc = Task.Run(() => PdfDocument.LoadFromFileAsync(file).AsTask()).Result;
        return (int)doc.PageCount;
    }
}
