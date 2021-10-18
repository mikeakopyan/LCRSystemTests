using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Collections;

namespace TestLCRTiming
{
    public class PWMData
    {
        
        const UInt32 encoderResetBit = (((UInt32)1) << 31);
        const UInt32 encoderABit = (((UInt32)1) << 30);
        const UInt32 encoderBBit = (((UInt32)1) << 29);

        float time_;
        UInt32 data_;

        public PWMData(float time, UInt32 data) { time_ = time; data_ = data; }

        public bool encoderA () { return (data_ & encoderABit) != 0;  }
        public bool encoderB () { return (data_ & encoderBBit) != 0;  }
        public bool encoderReset() { return (data_ & encoderResetBit) != 0; }
        public bool pwm(int laser) { return(data_ & (1 << laser)) != 0;  }
        public float time() { return time_;  }
    }

    class LaserPowerState
    {
        public uint encoder;
        public byte power;
        public bool on;
        public byte pwmPeriod;
        public byte pwmDuty;
    }
    
    class LaserPower
    {
        public float time;
        public List<float> adcLaserPower;
        public LaserPower() { adcLaserPower = new List<float>(21);  }
    }

    public partial class Form1 : Form
    {
        const byte laserBit = 0b00000001;
        const byte powerBit = 0b00000010;
        const byte pwmDutyBit = 0b00000100;
        const byte pwmPeridBit = 0b00001000;
        const byte SOTBit = 0b00010000;
        const byte EOTBit = 0b00100000;
        //byte resBit =      0b01000000;
        //byte parityBit =   0b10000000;

        List<PWMData> pwmData;
        List<float> encoderATransitions;
        List<float> encoderBTransitions;
        List<float> encoderTransisionsTime;
        List<int> encoderTransitionsIndex;

        List<LaserPowerState> laserPowerState;

        List<LaserPower> laserPower;

        public Form1()
        {
            InitializeComponent();
        }

        private void loadPWMData(string fileName)
        {
            pwmData = new List<PWMData>();
            int counter = 0;
            using (StreamReader file = new StreamReader(fileName))
            {
                string ln;
                while ((ln = file.ReadLine()) != null)
                {
                    string[] items = ln.Split(",");
                    float time = float.Parse(items[0]);
                    UInt32 data = (UInt32)Int32.Parse(items[1]);
                    pwmData.Add(new PWMData(time, data));
                    counter++;
                }
            }
            //MessageBox.Show(counter.ToString() + " items loaded");
        }

        private void loadAnalodData(string fileName)
        {
            laserPower = new List<LaserPower>();
            int counter = 0;
            using (StreamReader file = new StreamReader(fileName))
            {
                string ln;
                while ((ln = file.ReadLine()) != null)
                {
                    string[] items = ln.Split(",");
                    float time = float.Parse(items[0]);
                    LaserPower l = new LaserPower();
                    l.time = time;
                    for (int i=0; i<21; i++)
                    {
                        float v = float.Parse(items[i+1]);
                        l.adcLaserPower.Add(v);
                    }
                    laserPower.Add(l);
                    counter++;
                }
            }
            //MessageBox.Show(counter.ToString() + " items loaded");
        }


        byte[] textHeader;
        ArrayList blockOffset;
        ArrayList blockSize;
        int nLasers = 0;
        string fileName;
        byte[] allBytesInFile;

        int fileIndex = 0;
        uint currentPosition = 0;
        int numberOfOnes = 0;
        int trajectoryNumber = 0;
        //int lastFoundIndex = 0;
        bool positionErrors = false;
        bool parityErrors = false;
        bool formatErrors = false;
        bool unexpectedEOF = false;
        int packetNumber = 0;
        int onPacketPosition = -1;
        byte currentPWMPeriod = 0;
        byte currentPWMDuty = 0;
        byte currentLaserPower = 0;
        bool currentPowerState = false;
        bool prevPowerState = false;

        private bool setupVFLRFile(String filePath)
        {
            const int VFLRFileHeaderLength = 2000;
            const int VFLRFileBinaryHeaderVersion = 1;
            if (!File.Exists(filePath))
                return false;
            fileName = filePath;
            using (var stream = File.Open(filePath, FileMode.Open))
            {
                using (BinaryReader reader = new BinaryReader(stream))
                {
                    textHeader = reader.ReadBytes(VFLRFileHeaderLength);
                    byte format = reader.ReadByte();
                    if (format != VFLRFileBinaryHeaderVersion)
                    {
                        MessageBox.Show("Incorrect file format. Expecting 1, Actual: " + format.ToString());
                        return false;
                    }
                    byte nBlocks = reader.ReadByte();
                    blockOffset = new ArrayList();
                    blockSize = new ArrayList();
                    nLasers = nBlocks;
                    //byte[] bytes = reader.ReadBytes(nBlocks);
                    for (byte i = 0; i < nBlocks; i++)
                    {
                        UInt32 offset = reader.ReadUInt32();
                        UInt32 size = reader.ReadUInt32();
                        blockOffset.Add(offset);
                        blockSize.Add(size);
                    }
                }
            }
            return true;
        }

        private bool loadBlock(int index)
        {
            FileStream file_stream = new FileStream(fileName, FileMode.Open, FileAccess.Read);
            BinaryReader reader = new BinaryReader(file_stream);
            UInt32 offset = (UInt32)blockOffset[index];
            file_stream.Position = (long)offset;
            UInt32 size = (UInt32)blockSize[index];
            allBytesInFile = reader.ReadBytes((int)size);
            return true;
        }

        uint readPosition()
        {
            byte b1 = allBytesInFile[fileIndex++];
            byte b2 = allBytesInFile[fileIndex++];
            byte b3 = allBytesInFile[fileIndex++];
            updateNOA(b1);
            updateNOA(b2);
            updateNOA(b3);
            return ((uint)b3 << 16) + ((uint)b2 << 8) + (uint)b1;
        }

        private void updateNOA(byte b)
        {
            byte one = 1;
            for (int i = 0; i < 8; i++)
            {
                if ((b & one) != 0)
                    numberOfOnes++;
                one = (byte)(one << 1);
            }
        }

        private String toHex(byte b)
        {
            byte[] bb = new byte[1];
            bb[0] = b;
            return BitConverter.ToString(bb);
        }
        private char ascii(byte arg)
        {
            return (char)arg;
        }

        private String processPacket()
        {
            String strAscii = "";
            String strBytes = "";
            String strCommand = "";
            String strPositionError = "";
            String strParityError = "";
            String strFormatError = "";
            bool EOTCommand = false;
            strBytes += fileIndex.ToString("X8") + " ";
            byte command = allBytesInFile[fileIndex++];
            updateNOA(command);
            strBytes += toHex(command);
            strAscii += ascii(command);
            if (fileIndex >= allBytesInFile.Length)
            {
                formatErrors = true;
                unexpectedEOF = true;
                strFormatError = " <---- Unexpected EOF";
            }
            else
            {
                strBytes += "," + toHex(allBytesInFile[fileIndex]);
                strAscii += ascii(allBytesInFile[fileIndex]);
            }
            if (fileIndex + 1 >= allBytesInFile.Length)
            {
                formatErrors = true;
                unexpectedEOF = true;
                strFormatError = " <---- Unexpected EOF";
            }
            else
            {
                strBytes += "," + toHex(allBytesInFile[fileIndex + 1]);
                strAscii += ascii(allBytesInFile[fileIndex + 1]);
            }
            if (fileIndex + 2 >= allBytesInFile.Length)
            {
                formatErrors = true;
                unexpectedEOF = true;
                strFormatError = " <---- Unexpected EOF";
            }
            else
            {
                strBytes += "," + toHex(allBytesInFile[fileIndex + 2]);
                strAscii += ascii(allBytesInFile[fileIndex + 2]);
            }
            if (!unexpectedEOF)
            {
                uint position = readPosition();
                bool ignorePosition = (command & SOTBit) != 0 || (command & EOTBit) != 0;
                if (position >= 256 * 256 * 128)
                {
                    strPositionError += "  <---- POSITION ERROR: Value is out of range";
                    positionErrors = true;
                }
                if (position < currentPosition && !ignorePosition)
                {
                    strPositionError += "  <---- POSITION ERROR";
                    positionErrors = true;
                }
                currentPosition = position;
                strCommand += position.ToString().PadLeft(10);
                if ((command & laserBit) != 0)
                {
                    currentPowerState = true;
                    strCommand += "  ON";
                    if (onPacketPosition < 0)
                        onPacketPosition = (int)position;
                }
                else
                {
                    currentPowerState = false;
                    if (onPacketPosition < 0)
                    {
                        //strPositionError += "  <---- POSITION ERROR: OFF Packet";
                        //positionErrors = true;
                    }
                    //else
                    //    totalOnTicks += ((int)position - onPacketPosition);
                    onPacketPosition = -1;
                    strCommand += " OFF";
                }
                if ((command & powerBit) != 0)
                {
                    if (fileIndex >= allBytesInFile.Length)
                    {
                        formatErrors = true;
                        unexpectedEOF = true;
                        strFormatError = " <---- Unexpected EOF";
                    }
                    else
                    {
                        byte power = allBytesInFile[fileIndex++];
                        currentLaserPower = power;
                        updateNOA(power);
                        strCommand += ",W:" + power.ToString();
                        strBytes += "," + toHex(power);
                        strAscii += ascii(power);
                    }
                }
                if ((command & pwmDutyBit) != 0)
                {
                    if (fileIndex >= allBytesInFile.Length)
                    {
                        formatErrors = true;
                        unexpectedEOF = true;
                        strFormatError = " <---- Unexpected EOF";
                    }
                    else
                    {
                        byte duty = allBytesInFile[fileIndex++];
                        currentPWMDuty = duty;
                        updateNOA(duty);
                        strCommand += ",D:" + duty.ToString();
                        strBytes += "," + toHex(duty);
                        strAscii += ascii(duty);
                    }
                }
                if ((command & pwmPeridBit) != 0)
                {
                    if (fileIndex >= allBytesInFile.Length)
                    {
                        formatErrors = true;
                        unexpectedEOF = true;
                        strFormatError = " <---- Unexpected EOF";
                    }
                    else
                    {
                        byte period = allBytesInFile[fileIndex++];
                        currentPWMPeriod = period;
                        updateNOA(period);
                        strCommand += ",P:" + period.ToString();
                        strBytes += "," + toHex(period);
                        strAscii += ascii(period);
                    }
                }
                if ((command & SOTBit) != 0 && (command & EOTBit) != 0)
                    strCommand += ",End of layer";
                else
                {
                    if ((command & SOTBit) != 0)
                    {
                        strCommand += ",SOT";
                        trajectoryNumber++;
                    }
                    if ((command & EOTBit) != 0)
                    {
                        strCommand += ",EOT  " + trajectoryNumber.ToString();
                        packetNumber = 0;
                        EOTCommand = true;
                    }
                }
                if (numberOfOnes % 2 != 0)
                {
                    parityErrors = true;
                    strParityError = " <---- Parity Error";
                }
                if (packetNumber == 0 && EOTCommand == false && (command & SOTBit) == 0)
                {
                    formatErrors = true;
                    strFormatError = " <---- Not SOT";
                }
                EOTCommand = false;
            }
            bool laserOn = currentPowerState;
            if (currentPowerState != prevPowerState)
            {
                LaserPowerState laserState = new LaserPowerState();
                laserState.encoder = currentPosition;
                laserState.power = currentLaserPower;
                laserState.on = laserOn;
                laserState.pwmPeriod = currentPWMPeriod;
                laserState.pwmDuty = currentPWMDuty;
                laserPowerState.Add(laserState);
            }
            prevPowerState = currentPowerState;
            packetNumber++;
            String strReturn = "";
            strReturn += strCommand + strParityError + strPositionError + strFormatError;
            return strReturn;
        }

        private bool processAllPackets()
        {
            fileIndex = 0;
            currentPosition = 0;
            trajectoryNumber = 0;
            positionErrors = false;
            parityErrors = false;
            formatErrors = false;
            unexpectedEOF = false;
            packetNumber = 0;
            onPacketPosition = -1;
            if (allBytesInFile == null)
                return true;

            laserPowerState = new List<LaserPowerState>();

            while (fileIndex < allBytesInFile.Length && !unexpectedEOF)
            {
                int packetStartPosition = fileIndex;
                String str = processPacket();
                //if (show)
                //{
                //    lstIPackets.Items.Add(packetNumber.ToString() + "\t" + str);
                //    if (++packetsLoaded > 10000)
                //    {
                //        if (MessageBox.Show("Continue ?", "Too many packets", MessageBoxButtons.YesNo) == DialogResult.Yes)
                //        {
                //            packetsLoaded = 0;
                //        }
                //        else
                //            break;
                //    }
                //}
            }
            return !(positionErrors | parityErrors | formatErrors);
        }

        private void fillTransitions()
        {
            encoderATransitions = new List<float>();
            encoderBTransitions = new List<float>();
            encoderTransisionsTime = new List<float>();
            encoderTransitionsIndex = new List<int>();
            StreamWriter w = new StreamWriter(@"c:\Temp\Encoder.csv");
            StreamWriter w1 = new StreamWriter(@"c:\Temp\Laser1.csv");
            StreamWriter et = new StreamWriter(@"c:\Temp\EncoderTiming.csv");
            for (int i=0; i<pwmData.Count-1; i++)
            {
                bool transitionsA = pwmData[i].encoderA() != pwmData[i + 1].encoderA();
                bool transitionsB = pwmData[i].encoderB() != pwmData[i + 1].encoderB();
                if (transitionsA)
                    encoderATransitions.Add(pwmData[i].time());
                if (transitionsB)
                    encoderBTransitions.Add(pwmData[i].time());
                if (transitionsA || transitionsB)
                {
                    encoderTransisionsTime.Add(pwmData[i].time());
                    encoderTransitionsIndex.Add(i);
                    et.WriteLine(i.ToString() + "," + (1E6*pwmData[i].time()).ToString());
                }
                int a = pwmData[i].encoderA() ? 1 : 0;
                int b = pwmData[i].encoderB() ? 1 : 0;
                int ab = transitionsA || transitionsB ? 1 : 0;
                int r = pwmData[i].encoderReset() ? 1 : 0;
                int l1 = pwmData[i].pwm(0) ? 1 : 0;
                int l2 = pwmData[i].pwm(1) ? 1 : 0;
                int l3 = pwmData[i].pwm(2) ? 1 : 0;
                if (l1!=0)
                {
                    w1.WriteLine(i.ToString());
                }
                w.WriteLine(pwmData[i].time().ToString() + "," +
                    r.ToString() + "," + a.ToString() + "," +
                    b.ToString() + "," + ab.ToString() + "," +
                    l1.ToString() + "," + l2.ToString() + "," + l3.ToString());
            }
            w.Close();
            w1.Close();
            et.Close();
            StreamWriter wt = new StreamWriter(@"c:\Temp\Transitions.csv");
            int c = Math.Min(encoderATransitions.Count(), encoderBTransitions.Count());
            for (int i = 1; i < c-1; i++)
            {
                wt.WriteLine((1E6*(encoderATransitions[i + 1] - encoderATransitions[i])).ToString() + "," +
                    (1E6*(encoderBTransitions[i + 1] - encoderBTransitions[i])).ToString());
            }
            wt.Close();

            float minPeriod = 1000000.0f;
            float maxPeriod = 0.0f;
            int maxIndex = -1;
            int minIndex = -1;
            for (int i = 1; i < encoderBTransitions.Count - 1; i++)
            {
                float delta = encoderBTransitions[i + 1] - encoderBTransitions[i];
                if (delta > maxPeriod)
                {
                    maxPeriod = delta;
                    maxIndex = i;
                }
                if (delta < minPeriod)
                {
                    minIndex = i;
                    minPeriod = delta;
                }
            }
        }


        private void checkPWMTransitions()
        {
            List<int> encoderError = new List<int>();
            StreamWriter w = new StreamWriter(@"c:\Temp\EncoderErrors.csv");
            for (int i=0; i< laserPowerState.Count; i++)
            {
                int encoder = (int)laserPowerState[i].encoder;
                if (laserPowerState[i].on == false)
                    continue;
                int index = encoderTransitionsIndex[encoder];
                bool prevState = false;
                int trIndex = -1;
                for (int j=index-5; j<index+5; j++)
                {
                    bool pwm = pwmData[j].pwm(0);
                    if (pwm && !prevState)
                    {
                        trIndex = j;
                        break;
                    }
                    prevState = pwm;
                }
                w.WriteLine((trIndex - index).ToString());
                //encoderError.Add(trIndex+1 - index);
                //MessageBox.Show(index.ToString() + "  " + trIndex.ToString());
            }
            w.Close();
        }

        private void checkLaserPower()
        {
            List<float> laserPowerError = new List<float>();
            StreamWriter w = new StreamWriter(@"c:\Temp\LaserPowerErrors.csv");
            int encoderPowerOnIndex = -1;
            byte expectedLaserPower = 0;
            for (int i = 0; i < laserPowerState.Count; i++)
            {
                int encoder = (int)laserPowerState[i].encoder;
                if (laserPowerState[i].on)
                {
                    encoderPowerOnIndex = encoder;
                    expectedLaserPower = laserPowerState[i].power;
                    continue;
                }
                // laser power off
                int encoderCenter = (encoder + encoderPowerOnIndex) / 2;
                int index = encoderTransitionsIndex[encoderCenter];
                index = index / 5;  // adc data @ 1 MHz, digital data @ 1 MHz
                float avg = 0.0f;
                int n = 0;
                for (int j=index-10; j<=index+10; j++)
                {
                    avg += laserPower[j].adcLaserPower[1];
                    n++;
                }
                avg /= n;
                float expectedADC = 10.0f / 255.5f * ((float)expectedLaserPower+0.5f);
                float dv = avg - expectedADC;
                w.WriteLine(dv);
            }
            w.Close();
        }

        private void btnRun_Click(object sender, EventArgs e)
        {
            loadPWMData(@"C:\Temp\PortData0.csv");
            loadAnalodData(@"C:\Temp\AnalogData0.csv");
            fillTransitions();

            if (setupVFLRFile(@"C:\Projects\LCRTests\LCRTestsFiles\Rampup\VF-00000-0000_L00001_R01.vflr"))
            {
                if (loadBlock(0))
                {
                    processAllPackets();
                    checkPWMTransitions();
                }
                checkLaserPower();
            }
        }
    }
}
