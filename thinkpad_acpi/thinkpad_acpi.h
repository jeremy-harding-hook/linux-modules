/** \file thinkpad_acpi.h
 * This defines the public interface to the thinkpad_acpi.
 */

#ifndef THINKPAD_ACPI
#define THINKPAD_ACPI

#define TPACPI_LED_NUMLEDS 16

/**
 * \brief Represents an  led class device controlled by the thinkpad acpi.
 */
struct tpacpi_led_classdev {
	/**
	 * \brief The led class device
	 */
	struct led_classdev led_classdev;
	/**
	 * \brief An integer associated with the led, used to keep track of it.
	 *
	 * If positive, this is equal to the led's index. A value of -1 represents
	 * an led that is not supported on the current machine (probably because it
	 * doesn't actually exist).
	 */
	int led;
};

/**
 * \brief Gets an led controlled by the thinkpad acpi.
 */ 
struct tpacpi_led_classdev *tpacpi_get_led(unsigned int index);

#endif /* THINKPAD_ACPI */
