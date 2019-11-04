using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Drawing;
using Color = System.Drawing.Color;
using System.IO;

namespace GetImageWPF
{
    /// <summary>
    /// Logica di interazione per MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        string dstPrefix = "slot_";
        static String CRLF = "\r\n";
        static int MAX_COLUMN = 16;
        Boolean isTranparent = false;
        int transparentColor = 0xFFFF; // white (not used)
        double width;
        double height;
        string srcFolder = "images\\";
        string dstFolder = "src\\";

        Boolean isUnionSize = true;
        String[] imageNames = { "seven", "bar", "logo", "cherry", "orange", "lemon" };


        public MainWindow()
        {
            InitializeComponent();
        }

        private void btnLoadImage(object sender, RoutedEventArgs e)
        {
            btnGo.IsEnabled = false;
            StringBuilder sb = new StringBuilder();
            StringBuilder sbArray = new StringBuilder();

            StreamWriter fileOut;
            foreach (String name in imageNames)
            {
                Console.WriteLine(name + ".png");
                String srcPath = "images\\" + dstPrefix + name + ".png";
                String dstPath = "src\\" + dstPrefix + name + ".h";

                //string txt = dumpImage("images\\slot_bar.png", null);
                String text = dumpImage(srcPath, dstPrefix + name);

                if (!Directory.Exists("src"))
                    Directory.CreateDirectory("src");

                fileOut = new StreamWriter(dstPath);
                fileOut.Write(text);
                fileOut.Close();

                sb.Append("#include \"" + dstPrefix + name + ".h\"" + CRLF);
                sbArray.Append("\t" + dstPrefix + name + "," + CRLF);
            }

            if (isUnionSize)
            {
                sb.Append(CRLF);
                sb.Append("#define SYM_WIDTH " + width + CRLF);
                sb.Append("#define SYM_HEIGHT " + height + CRLF);
                sb.Append("#define SYM_COUNT " + imageNames.Length + CRLF);
            }

            sb.Append(CRLF);

            if (isTranparent)
            {
                sb.Append("#define COLOR_TRANSP " + String.Format("0x{0,4:X4},", transparentColor) + CRLF);
                sb.Append(CRLF);
            }

            sb.Append("const uint16_t *slot_symbols[] = {" + CRLF);
            sb.Append(sbArray);
            sb.Append("};" + CRLF);

            fileOut = new StreamWriter(dstFolder + "slot_symbols.h");
            fileOut.Write(sb.ToString());
            fileOut.Close();

            Console.WriteLine("done.");
            btnGo.IsEnabled = true;
            btnGo.Content = "Fatto!";

        }

        String dumpImage(String path, String varName)
        {
            Bitmap bi = new Bitmap(path);
            width = bi.Width;
            height = bi.Height;

            //Boolean hasAlpha = bi.getType() == BufferedImage.TYPE_4BYTE_ABGR;

            if (varName == null)
                varName = "out.h";

            StringBuilder sb = new StringBuilder();
            sb.Append("#include <Arduino.h>" + CRLF);
            sb.Append(CRLF);

            if (!isUnionSize)
            {
                sb.Append("#define " + varName.ToUpper() + "_WIDTH " + width + CRLF);
                sb.Append("#define " + varName.ToUpper() + "_HEIGHT " + height + CRLF);
                sb.Append(CRLF);
            }

            sb.Append("const uint16_t PROGMEM " + varName + "[] = {" + CRLF);

            int col = 0;
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    if (col == 0)
                    {
                        sb.Append("\t"); // indent
                    }

                    Color punto = bi.GetPixel(x, y);
                    int color16;
                    if (punto == Color.Transparent)
                    {
                        color16 = transparentColor;
                    }
                    else
                    {
                        color16 = color32To16(punto.ToArgb());
                    }
                    sb.Append(String.Format("0x{0,4:X4},", color16));
                    if (++col >= MAX_COLUMN)
                    {
                        col = 0;
                        sb.Append(CRLF);
                    }
                }
            }
            if (col != 0)
            {
                sb.Append(CRLF);
            }
            sb.Append("};" + CRLF);

            return sb.ToString();

        }

        int color32To16(int rgb)
        {
            return color565((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }

        int color565(int r, int g, int b)
        {
            return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

    }
}
