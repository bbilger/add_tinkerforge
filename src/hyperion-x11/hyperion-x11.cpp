
// QT includes
#include <QCoreApplication>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "ProtoWrapper.h"
#include "X11Wrapper.h"

using namespace vlofgren;

// save the image as screenshot
void saveScreenshot(const char * filename, const Image<ColorRgb> & image)
{
	// store as PNG
	QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
	pngImage.save(filename);
}

int main(int argc, char ** argv)
{
	QCoreApplication app(argc, argv);

	try
	{
		// create the option parser and initialize all parameters
		OptionsParser optionParser("X11 capture application for Hyperion");
		ParameterSet & parameters = optionParser.getParameters();

		IntParameter           & argCropWidth       = parameters.add<IntParameter>          (0x0, "crop-width",       "Number of pixels to crop from the left and right sides in the picture before decimation [default=0]");
		IntParameter           & argCropHeight      = parameters.add<IntParameter>          (0x0, "crop-height",      "Number of pixels to crop from the top and the bottom in the picture before decimation [default=0]");
		IntParameter           & argSizeDecimation  = parameters.add<IntParameter>          ('s', "size-decimator",   "Decimation factor for the output size [default=1]");
		SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",       "Take a single screenshot, save it to file and quit");
		StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",          "Set the address of the hyperion server [default: 127.0.0.1:19445]");
		IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",         "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
		SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",       "Do not receive and check reply messages from Hyperion");
		SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",             "Show this help message and exit");

		// set defaults
		argCropWidth.setDefault(0);
		argCropHeight.setDefault(0);
		argSizeDecimation.setDefault(1);
		argAddress.setDefault("127.0.0.1:19445");
		argPriority.setDefault(800);

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}

		// Create the X11 grabbing stuff
		X11Wrapper x11Wrapper(argCropWidth.getValue(), argCropHeight.getValue(), argSizeDecimation.getValue());

		if (argScreenshot.isSet())
		{
			// Capture a single screenshot and finish
			const Image<ColorRgb> & screenshot = x11Wrapper.getScreenshot();
			saveScreenshot("screenshot.png", screenshot);
		}
		else
		{
			// Create the Proto-connection with hyperiond
			ProtoWrapper protoWrapper(argAddress.getValue(), argSkipReply.isSet());

			// Connect the screen capturing to the proto processing
			QObject::connect(&x11Wrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &protoWrapper, SLOT(process(Image<ColorRgb>)));

			// Start the capturing
			x11Wrapper.start();

			// Start the application
			app.exec();
		}
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}