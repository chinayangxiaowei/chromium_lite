// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.LocaleList;
import android.support.test.filters.SmallTest;
import android.text.TextUtils;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.util.JSUtils;
import org.chromium.base.LocaleUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.Arrays;
import java.util.Locale;
import java.util.regex.Pattern;

/**
 * Tests for Accept Language implementation.
 */
public class AcceptLanguageTest extends AwTestBase {
    private TestAwContentsClient mContentsClient;
    private AwContents mAwContents;

    private EmbeddedTestServer mTestServer;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mContentsClient = new TestAwContentsClient();
        mAwContents = createAwTestContainerViewOnMainSync(mContentsClient).getAwContents();

        mTestServer = EmbeddedTestServer.createAndStartServer(getInstrumentation().getContext());
    }

    @Override
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
        super.tearDown();
    }

    private static final Pattern COMMA_AND_OPTIONAL_Q_VALUE =
            Pattern.compile("(?:;q=[^,]+)?(?:,|$)");

    /**
     * Extract the languages from the Accept-Language header.
     *
     * The Accept-Language header can have more than one language along with optional quality
     * factors for each, e.g.
     *
     *  "de-DE,en-US;q=0.8,en-UK;q=0.5"
     *
     * This function extracts only the language strings from the Accept-Language header, so
     * the example above would yield the following:
     *
     *  ["de-DE", "en-US", "en-UK"]
     *
     * @param raw String containing the raw Accept-Language header
     * @return A list of languages as Strings.
     */
    private String[] getAcceptLanguages(String raw) {
        return COMMA_AND_OPTIONAL_Q_VALUE.split(maybeStripDoubleQuotes(raw));
    }

    /**
     * Verify that the Accept Language string is correct.
     */
    @SmallTest
    @Feature({"AndroidWebView"})
    public void testAcceptLanguage() throws Throwable {
        getAwSettingsOnUiThread(mAwContents).setJavaScriptEnabled(true);

        // This should yield a lightly formatted page with the contents of the Accept-Language
        // header, e.g. "en-US" or "de-DE,en-US;q=0.8", as the only text content.
        String url = mTestServer.getURL("/echoheader?Accept-Language");
        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        String[] acceptLanguages = getAcceptLanguages(
                JSUtils.executeJavaScriptAndWaitForResult(
                        this, mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                        "document.body.textContent"));
        assertEquals(LocaleUtils.getDefaultLocaleString(), acceptLanguages[0]);

        String[] acceptLanguagesJs = getAcceptLanguages(
                JSUtils.executeJavaScriptAndWaitForResult(
                        this, mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                        "navigator.languages.join(',')"));
        assertEquals(acceptLanguagesJs.length, acceptLanguages.length);
        for (int i = 0; i < acceptLanguagesJs.length; ++i) {
            assertEquals(acceptLanguagesJs[i], acceptLanguages[i]);
        }

        // Test locale change at run time
        Locale.setDefault(new Locale("de", "DE"));
        AwContents.updateDefaultLocale();
        mAwContents.getSettings().updateAcceptLanguages();

        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        acceptLanguages = getAcceptLanguages(
                JSUtils.executeJavaScriptAndWaitForResult(
                        this, mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                        "document.body.textContent"));
        assertEquals(LocaleUtils.getDefaultLocaleString(), acceptLanguages[0]);
    }

    /**
     * Verify that the Accept Languages string is correct.
     * When default locales do not contain "en-US" or "en-us",
     * "en-US" should be added with lowest priority.
     */
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.N)
    @SuppressLint("NewApi")
    @Feature({"AndroidWebView"})
    public void testAcceptLanguagesWithenUS() throws Throwable {
        getAwSettingsOnUiThread(mAwContents).setJavaScriptEnabled(true);

        // This should yield a lightly formatted page with the contents of the Accept-Language
        // header, e.g. "en-US" or "de-DE,en-US;q=0.8", as the only text content.
        String url = mTestServer.getURL("/echoheader?Accept-Language");
        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        String[] acceptLanguages = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(
                this, mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "document.body.textContent"));
        assertEquals(
                LocaleUtils.getDefaultLocaleListString(), TextUtils.join(",", acceptLanguages));

        String[] acceptLanguagesJs = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(
                this, mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "navigator.languages.join(',')"));
        assertEquals(acceptLanguagesJs.length, acceptLanguages.length);
        for (int i = 0; i < acceptLanguagesJs.length; ++i) {
            assertEquals(acceptLanguagesJs[i], acceptLanguages[i]);
        }

        // Test locales that contain "en-US" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("en", "US")));
        AwContents.updateDefaultLocale();
        mAwContents.getSettings().updateAcceptLanguages();

        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        acceptLanguages = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(this,
                mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "document.body.textContent"));
        assertEquals(
                LocaleUtils.getDefaultLocaleListString(), TextUtils.join(",", acceptLanguages));

        // Test locales that contain "en-us" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("en", "us")));
        AwContents.updateDefaultLocale();
        mAwContents.getSettings().updateAcceptLanguages();

        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        acceptLanguages = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(this,
                mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "document.body.textContent"));
        assertEquals(
                LocaleUtils.getDefaultLocaleListString(), TextUtils.join(",", acceptLanguages));

        // Test locales that do not contain "en-us" or "en-US" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("ja", "JP")));
        AwContents.updateDefaultLocale();
        mAwContents.getSettings().updateAcceptLanguages();

        loadUrlSync(mAwContents, mContentsClient.getOnPageFinishedHelper(), url);

        acceptLanguages = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(this,
                mAwContents, mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "document.body.textContent"));
        String[] acceptLangs = Arrays.copyOfRange(acceptLanguages, 0, acceptLanguages.length - 1);
        assertEquals(LocaleUtils.getDefaultLocaleListString(), TextUtils.join(",", acceptLangs));
    }
}
