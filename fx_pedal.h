/** @mainpage
 *
 * @section introduction Introduction
 * FX pedals are an extremely popular industry, and as the performance of embedded devices
 * becomes better and better, digital FX pedals are becoming increasingly popular. Most
 * early FX pedals were analog, and many guitarists criticized digital pedals as sounding
 * more "artificial" and "detached". However, the latest digital pedals produce FX that
 * are currently impossible with analog circuits, and the sound quality of digital pedals
 * is drastically improving. The goal of this project was to create a basic FX pedal using
 * the Raspberry Pi and a Tiva C. The included FX are very simple, but serve as a proof of
 * concept and demonstrate the power of the Raspberry Pi. More advanced FX could be
 * produced if a dedicated DSP chip were used.
 *
 * @section hardware Hardware
 * The pedal is built on a Raspberry Pi and uses a Tiva C for its user interface. On top
 * of the Raspberry Pi sits a Wolfson audio card [1] with 1/8" input and output jacks. The
 * Tiva C is hooked up to the Raspberry Pi's J8 header, which is used for serial
 * interfaces. Pins PB0 (Rx) and PB1 (Tx) communicate over UART with the Tx and Rx pins
 * on the Raspberry Pi. The Tiva C uses 2 buttons (SW1 and SW2) to capture user input
 * and send the appropriate data to the Pi.
 *
 * @section software Software
 * The Raspberry Pi is running a distribution of Raspbian issued by Wolfson [1]. This
 * distribution contains the drivers that register the sound card as an ALSA device. ALSA
 * is a framework for registering audio devices on a Linux computer that provides a common
 * interface for reading and writing data to them [2]. On top of the ALSA library, the
 * JACK Audio Connection Kit [3] provides an even more uniform API for communicating with
 * the card. Once a JACK server is running, the user can select a frame size to balance
 * computational requirements with latency. Every time a frame is received, a process
 * function is called. The developer has access to a pointer to all of the samples in the
 * frame, and can choose to do whatever he or she wants with these samples and then copies
 * them to an output port.
 *
 * @section uart UART Controller
 * The user interface is controlled by a Tiva C. The Tiva C is connected to the Raspberry
 * Pi over UART. The Raspberry Pi reads the serial port to which the Tiva C sends UART
 * data on a separate thread from the FX processor, in order to maintain uninterrupted,
 * real-time audio processing.
 *
 * The Tiva C has two buttons. The left button is used to cycle through the different FX,
 * which are covered in the @ref fx "FX Processor" section. The second button is used as
 * a tap tempo. This is a commonly used control in FX pedals. The user can tap the button
 * at the speed he/she wants the delay to occur. The controller will attempt to match the
 * echo rate to the exact rate at which the button was pressed. Some rounding will occur
 * due to the frame size, but this difference is usually unnoticeable. The controller will
 * consider up to 4 most recent taps to calculate the tempo. If the button has not been
 * pressed for more than 2 seconds, the tap tempo will start fresh.
 * 
 * @section delay Delay Buffer
 * Delay (or echo) is a common effect that guitarists use to create a repeated decaying
 * echo of whatever they play. The delay effect creates an ambient sound that can really
 * enhance a particular part of a song or solo. There are three main parameters that can
 * change the way a delay sounds.
 *
 * - Mix (or level) - determines how loud the delay is compared to the dry signal.
 * - Decay - determines how quickly the signal will decay (or how many times it will repat)
 * - Delay (or tempo) - determines the duration of time that passes before the signal repeats
 *
 * Adjusting these three parameters can create a wide variety of types of delay. A long
 * delay with a fast decay rate can make it sound like two guitarists are playing the same
 * song, but one of them is following the other. A short delay time with a medium decay 
 * rate can create a reverberation which is described in the \ref reverb "reverb" section
 * below. A long decay rate with a medium delay time and a low level can create an ambient
 * background noise that sounds like a blend of all of the notes the guitarist is playing.
 * There are several things guitarists can do with the delay effect, which is why it is
 * one of the most popular effects out there. The delay effect in this project allows for
 * other FX (described in the \ref fx "FX Processor" section below) to be applied to the
 * signal when it is repeated. The dry signal comes through clean, but the echo'd signals
 * will have FX applied to them. Doing so can produce some unique sounds depending on how
 * the guitar is played.
 *
 * @section fx FX Processor
 * The FX Processor is responsible for processing audio on a sample by sample basis. The
 * class defines one function, `process`, which processes audio on a sample-by-sample
 * basis. The FX processing logic was separated from the delay buffer so that it can be
 * used independently of the delay if desired. The following sections explain which FX
 * the class is capable of processing.
 *
 * @subsection overdrive Overdrive
 * Overdrive is a classic effect used to simulate tubes on an amplifier reaching their
 * limit of amplification. This caused the tubes to round off the higher end of the audio
 * coming through. The rounding causes slight distortion in the signal. The algorithm in
 * this project uses a formula found in [4]. The equation is:
 *
 * \f$x=\frac{(1+k)x}{1+k*abs(x)}\f$  <br />
 * \f$k=\frac{2a}{1-a}\f$  <br />
 * \f$a=sin(\frac{drive+.01}{1.01}\frac{\pi}{2})\f$  
 *
 * Where \f$x\f$ is the sample, and \f$drive\f$ is a value between 0 and 1, specifying how
 * much overdrive is desired.
 *
 * This algorith differs from hard distortion, which is described below.
 *
 * @subsection distortion Distortion
 * Distortion is similar to overdrive, but generally has a harsher sound than overdrive.
 * There are many different types of distortions, and they are generally made with analog
 * circuits, but a simple algorithm is to simply amplify the signal and clip it. In this
 * class, we multiply the signal (whose range is generally between -0.2 and 0.2) by a
 * number from 1 to 5, and clip it at 0.2.
 *
 * @subsection tremolo Tremolo
 * Tremolo is a technique used by instrumentalists where a single note was repeatedly
 * picked, bowed, or played very quickly. The tremolo effect produces a similar sound on a
 * sustained note by repeatedly cutting and restoring the volume of the output. This
 * allows a player to simply hold a note and get the effect of tremolo without having to
 * perform the technique.
 *
 * @subsection reverb Reverb
 * Reverberation is a phenomenon know to occur in large rooms without much dampening (i.e.
 * hard floors and walls). The phenomenon is observed when a loud sound occurs and bounces
 * off the many surfaces in the room creating an echo. Usually, the sound will reach
 * different surfaces at different times, creating a layered echoing effect that differs
 * from delay. The effect was originally recreated by placing an amplifier in a large,
 * undamped room, and recording the natural effect in that room. Afterwards, reverb units
 * were placed in some amplifiers, which consisted of a spring that would vibrate as sound
 * came through the amplifier [5]. A very simple digital reverb effect was created in
 * cartoons by simply producing an echo with a very short repeat delay. That is the effect
 * used in this class's reverb algorithm.
 *
 * @subsection wah Wah
 * A wah effect simulates the natural filter that occurs from someones lips when they say
 * the word "WAH". The mouth can be thought of as a moving bandpass filter which sweeps
 * from low to high frequency. In this class's wah algorithm, a moving bandpass filter is 
 * generated using the following equations:
 *
 * \f$y_l(n) = F_1y_b(n) + y_l(n-1)\f$  <br />
 * \f$y_b(n) = F_1y_h(n) + y_b(n-1)\f$  <br />
 * \f$y_h(n) = x(n) - y_l(n-1) - Q_1y_b(n-1)\f$  <br />
 * \f$F_1 = 2sin(\pi \frac{f_c}{f_s})\f$
 *
 * where \f$Q_1\f$ determines the size of the pass band and is chosen to be 0.1 here [6].
 *
 * 1. http://www.element14.com/community/community/raspberry-pi/raspberry-pi-accessories/wolfson_pi
 * 2. http://www.alsa-project.org/main/index.php/Main_Page
 * 3. http://jackaudio.org
 * 4. http://ses.library.usyd.edu.au/bitstream/2123/7624/2/DESC9115_DAS_Assign02_310106370.pdf
 * 5. http://www.soundonsound.com/sos/Oct01/articles/advancedreverb1.asp
 * 6. http://www.cs.cf.ac.uk/Dave/CM0268/PDF/10_CM0268_Audio_FX.pdf
 */