#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <elf.h>
#include <fcntl.h>

#include <string>
#include <jni.h>

#include <android/log.h>
#include <android_runtime/AndroidRuntime.h>

#include "JniHelper.h"


// ./inject com.tencent.mm /data/local/tmp/libhello.so


#define LOG_TAG "DEBUG"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)  
 


extern "C" char* Jstring2CStr(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = (env)->FindClass( "java/lang/String");
    jstring strencode = (env)->NewStringUTF("GB2312");
    jmethodID mid = (env)->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray)(env)->CallObjectMethod(jstr, mid, strencode); // String .getByte("GB2312");
    jsize alen = (env)->GetArrayLength(barr);
    jbyte* ba = (env)->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1); //new   char[alen+1]; "\0"
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    (env)->ReleaseByteArrayElements(barr, ba, 0); //释放内存
 
    return rtn;
}

jobject getGlobalContext(JNIEnv *env) {
//    Object app = Class.forName("android.app.ActivityThread").getMethod("currentApplication").invoke(null, (Object[]) null);
    //获取 ActivityThread 的实例对象
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
    //获取 Application，也就是全局的 Context
    jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(at, getApplication);
    return context;
}

void printJobject(JNIEnv *env, jobject obj) {
    jclass cls = env->GetObjectClass(obj);
    jmethodID toString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    jstring stringVal = (jstring)env->CallObjectMethod(obj, toString);
    LOGD("Hello - To String: %s\n", Jstring2CStr(env, stringVal));
}

jclass loadClassByPathClassLoader(JNIEnv *env, const char *className){
    jobject context = getGlobalContext(env);
    jclass contextClazz = env->GetObjectClass(context);
    jmethodID getClassLoaderMethod = env->GetMethodID(contextClazz, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject pathClsLoader = env->CallObjectMethod(context, getClassLoaderMethod);

    printJobject(env, pathClsLoader);

    jobject classLoader = pathClsLoader;
    jclass clazzCL = env->GetObjectClass(classLoader);
    jmethodID loadClass = env->GetMethodID(clazzCL,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");

    jstring param = env->NewStringUTF(className);
    jclass tClazz = (jclass)env->CallObjectMethod(classLoader,loadClass,param);

    return tClazz;
}



static JavaVM *g_JavaVM;
extern "C" JNIEnv *GetEnv() {
 	int status;
    JNIEnv *envnow = NULL;
    status = g_JavaVM->GetEnv((void **)&envnow, JNI_VERSION_1_4);
    if(status < 0) {
        status = g_JavaVM->AttachCurrentThread(&envnow, NULL);
        if(status < 0) {
            return NULL;
        } else {
        	LOGD("Hello GetEnv(). JNIEnv Attached thread success");
        }
        //g_bAttatedT = true;
    }
    return envnow;
}

extern "C" int hook_entry(char * arg){
    LOGD("Hello - Hook success, pid = %d, parameter = %s\n", getpid(), arg);

    g_JavaVM = android::AndroidRuntime::getJavaVM();

    JniHelper::setJavaVM(g_JavaVM);

    JNIEnv* env = GetEnv(); // JniHelper::getEnv(); 


    jclass mmModelQClass = loadClassByPathClassLoader(env, "com.tencent.mm.model.q");
    if (mmModelQClass != NULL) {
        LOGD("Hello - Find Class Success\n");
    } else {
        LOGD("Hello - Find Class Failed\n");
        return 1;
    }
    jmethodID soMethod = env->GetStaticMethodID(mmModelQClass, "SO", "()Ljava/lang/String;");
    LOGD("Hello - Get method success");
    jstring accountString = (jstring)env->CallStaticObjectMethod(mmModelQClass, soMethod);
    LOGD("Hello - Get the account success: %s\n", Jstring2CStr(env, accountString));

    return 0;
}
