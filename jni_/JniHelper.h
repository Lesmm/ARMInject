#ifndef __ANDROID_JNI_HELPER_H__
#define __ANDROID_JNI_HELPER_H__

#include <jni.h>
#include <string>

typedef struct JniMethodInfo_ {
    JNIEnv * env;
    jclass classID;
    jmethodID methodID;
} JniMethodInfo;

class JniHelper {
public:
    static void setJavaVM(JavaVM *javaVM);
    static JavaVM* getJavaVM();
    static JNIEnv* getEnv();

    static bool setClassLoaderFrom(jobject activityInstance);
    static bool getStaticMethodInfo(JniMethodInfo &methodinfo,
            const char *className, const char *methodName,
            const char *paramCode);
    static bool getMethodInfo(JniMethodInfo &methodinfo, const char *className,
            const char *methodName, const char *paramCode);

    static std::string jstring2string(jstring str);

    static int ClearException(JNIEnv *jenv);
    static jclass getAppClass(JNIEnv *jenv,const char *apn);
    static jclass findAppClass(JNIEnv *jenv,const char *apn);

    static jmethodID loadclassMethod_methodID;
    static jobject classloader;

    static jobject getGlobalContext(JNIEnv *env);
    static void printJobject(JNIEnv *env, jobject obj);
    static jclass loadClassByPathClassLoader(JNIEnv *env, const char *className);
    static char* Jstring2CStr(JNIEnv* env, jstring jstr);

private:
    static JNIEnv* cacheEnv(JavaVM* jvm);

    static bool getMethodInfo_DefaultClassLoader(JniMethodInfo &methodinfo,
            const char *className, const char *methodName,
            const char *paramCode);

    static JavaVM* _psJavaVM;
};

#endif // __ANDROID_JNI_HELPER_H__

