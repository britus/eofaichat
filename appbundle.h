#pragma once

extern "C" {
/**
 * @brief Get the bundle version
 * @return a string
 */
const char *getBundleVersion();

/**
 * @brief Get the bundle build number
 * @return a string
 */
const char *getBuildNumber();
}
