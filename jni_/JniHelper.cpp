/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies Inc.
http://www.cocos2d-x.org
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "JniHelper.h"
#include <android/log.h>
#include <string.h>
#include <pthread.h>
 
#define  LOG_TAG    "JniHelper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  ALOG(...) __android_log_print(ANDROID_LOG_VERBOSE, __VA_ARGS__)
 
static pthread_key_t g_key;
 
jclass _getClassID(const char *className) {
    if (NULL == className) {
        return NULL;
    }
 
    JNIEnv* env = JniHelper::getEnv();
 
    jstring _jstrClassName = env->NewStringUTF(className);
 
    jclass _clazz = (jclass) env->CallObjectMethod(JniHelper::classloader,
                                                   JniHelper::loadclassMethod_methodID,
                                                   _jstrClassName);
 
    if (NULL == _clazz) {
        LOGE("JniHelper - Classloader failed to find class of %s", className);
        env->ExceptionClear();
    }
 
    env->DeleteLocalRef(_jstrClassName);
        
    return _clazz;
}
 
void _detachCurrentThread(void* a) {
    JniHelper::getJavaVM()->DetachCurrentThread();
}
 

JavaVM* JniHelper::_psJavaVM = NULL;
jmethodID JniHelper::loadclassMethod_methodID = NULL;
jobject JniHelper::classloader = NULL;

JavaVM* JniHelper::getJavaVM() {
    pthread_t thisthread = pthread_self();
    LOGD("JniHelper::getJavaVM(), pthread_self() = %ld", thisthread);
    return _psJavaVM;
}

void JniHelper::setJavaVM(JavaVM *javaVM) {
    pthread_t thisthread = pthread_self();
    LOGD("JniHelper::setJavaVM(%p), pthread_self() = %ld", javaVM, thisthread);
    _psJavaVM = javaVM;

    pthread_key_create(&g_key, _detachCurrentThread);
}

JNIEnv* JniHelper::cacheEnv(JavaVM* jvm) {
    JNIEnv* _env = NULL;
    // get jni environment
    jint ret = jvm->GetEnv((void**)&_env, JNI_VERSION_1_4);
    
    switch (ret) {
    case JNI_OK :
        // Success!
        LOGD("JniHelper::cacheEnv() Already Attached thread success");
        pthread_setspecific(g_key, _env);
        return _env;
            
    case JNI_EDETACHED :
        // Thread not attached
        if (jvm->AttachCurrentThread(&_env, NULL) < 0) {
            LOGE("JniHelper - Failed to get the environment using AttachCurrentThread()");

            return NULL;
        } else {
            // Success : Attached and obtained JNIEnv!
            LOGD("JniHelper::cacheEnv() Attached thread success");
            pthread_setspecific(g_key, _env);
            return _env;
        }
            
    case JNI_EVERSION :
        // Cannot recover from this error
        LOGE("JniHelper - JNI interface version 1.4 not supported");
    default :
        LOGE("JniHelper - Failed to get the environment using GetEnv()");
        return NULL;
    }
}

JNIEnv* JniHelper::getEnv() {
    JNIEnv *_env = (JNIEnv *)pthread_getspecific(g_key);
    if (_env == NULL)
        _env = JniHelper::cacheEnv(_psJavaVM);
    return _env;
}

bool JniHelper::setClassLoaderFrom(jobject activityinstance) {
    JniMethodInfo _getclassloaderMethod;
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_getclassloaderMethod,
                                                     "android/content/Context",
                                                     "getClassLoader",
                                                     "()Ljava/lang/ClassLoader;")) {
        return false;
    }

    jobject _c = JniHelper::getEnv()->CallObjectMethod(activityinstance,
                                                                _getclassloaderMethod.methodID);

    if (NULL == _c) {
        return false;
    }

    JniMethodInfo _m;
    if (!JniHelper::getMethodInfo_DefaultClassLoader(_m,
                                                     "java/lang/ClassLoader",
                                                     "loadClass",
                                                     "(Ljava/lang/String;)Ljava/lang/Class;")) {
        return false;
    }

    JniHelper::classloader = JniHelper::getEnv()->NewGlobalRef(_c);
    JniHelper::loadclassMethod_methodID = _m.methodID;

    return true;
}

bool JniHelper::getStaticMethodInfo(JniMethodInfo &methodinfo,
                                    const char *className, 
                                    const char *methodName,
                                    const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        LOGE("JniHelper - Failed to get JNIEnv");
        return false;
    }
        
    jclass classID = _getClassID(className);
    if (! classID) {
        LOGE("JniHelper - Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetStaticMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("JniHelper - Failed to find static method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }
        
    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;
    return true;
}

bool JniHelper::getMethodInfo_DefaultClassLoader(JniMethodInfo &methodinfo,
                                                 const char *className,
                                                 const char *methodName,
                                                 const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return false;
    }

    jclass classID = env->FindClass(className);
    if (! classID) {
        LOGE("JniHelper - Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("JniHelper - Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

bool JniHelper::getMethodInfo(JniMethodInfo &methodinfo,
                              const char *className,
                              const char *methodName,
                              const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return false;
    }

    jclass classID = _getClassID(className);
    if (! classID) {
        LOGE("JniHelper - Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGE("JniHelper - Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = env;
    methodinfo.methodID = methodID;

    return true;
}

std::string JniHelper::jstring2string(jstring jstr) {
    if (jstr == NULL) {
        return "";
    }
    
    JNIEnv *env = JniHelper::getEnv();
    if (!env) {
        return NULL;
    }

    const char* chars = env->GetStringUTFChars(jstr, NULL);
    std::string ret(chars);
    env->ReleaseStringUTFChars(jstr, chars);

    return ret;
}



int JniHelper::ClearException(JNIEnv *jenv) {
    jthrowable exception = jenv->ExceptionOccurred();
    if (exception != NULL) {
        jenv->ExceptionDescribe();
        jenv->ExceptionClear();
        return true;
    }
    return false;
}

jclass JniHelper::getAppClass(JNIEnv *jenv,const char *apn) {
    jclass clazzTarget = jenv->FindClass(apn);
    if (ClearException(jenv)) {
        ALOG("Exception","ClassMethodHook[Can't find class:%s in bootclassloader",apn);
        clazzTarget = findAppClass(jenv, apn);
        if(clazzTarget == NULL){
            ALOG("Exception","%s","Error in findAppClass");
//            return NULL;
            clazzTarget = loadClassByPathClassLoader(jenv, apn);
        }
    }
    return clazzTarget;
}

jclass JniHelper::findAppClass(JNIEnv *jenv,const char *apn) {
    //获取Loaders
    jclass clazzApplicationLoaders = jenv->FindClass("android/app/ApplicationLoaders");
    jthrowable exception = jenv->ExceptionOccurred();
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","No class : %s", "android/app/ApplicationLoaders");
        return NULL;
    }
    jfieldID fieldApplicationLoaders = jenv->GetStaticFieldID(clazzApplicationLoaders,"gApplicationLoaders","Landroid/app/ApplicationLoaders;");
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","No Static Field :%s","gApplicationLoaders");
        return NULL;
    }
    jobject objApplicationLoaders = jenv->GetStaticObjectField(clazzApplicationLoaders,fieldApplicationLoaders);
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","GetStaticObjectField is failed [%s","gApplicationLoaders");
        return NULL;
    }
    // jfieldID fieldLoaders = jenv->GetFieldID(clazzApplicationLoaders,"mLoaders","Landroid/util/ArrayMap<Ljava/lang/String;Ljava/lang/ClassLoader;>;");
    jfieldID fieldLoaders = jenv->GetFieldID(clazzApplicationLoaders,"mLoaders","Ljava/util/Map;");
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","No Field :%s","mLoaders");
        return NULL;
    }
    jobject objLoaders = jenv->GetObjectField(objApplicationLoaders,fieldLoaders);
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","No object :%s","mLoaders");
        return NULL;
    }
    //提取map中的values
    jclass clazzHashMap = jenv->GetObjectClass(objLoaders);
    jmethodID methodValues = jenv->GetMethodID(clazzHashMap,"values","()Ljava/util/Collection;");
    jobject values = jenv->CallObjectMethod(objLoaders,methodValues);

    jclass clazzValues = jenv->GetObjectClass(values);
    jmethodID methodToArray = jenv->GetMethodID(clazzValues,"toArray","()[Ljava/lang/Object;");
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","No Method:%s","toArray");
        return NULL;
    }

    jobjectArray classLoaders = (jobjectArray)jenv->CallObjectMethod(values,methodToArray);
    if (ClearException(jenv)) {
        ALOG("JniHelper - Exception","CallObjectMethod failed :%s","toArray");
        return NULL;
    }

        int size = jenv->GetArrayLength(classLoaders);

        for(int i = 0 ; i < size ; i ++){
            jobject classLoader = jenv->GetObjectArrayElement(classLoaders,i);
            jclass clazzCL = jenv->GetObjectClass(classLoader);
            jmethodID loadClass = jenv->GetMethodID(clazzCL,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
            jstring param = jenv->NewStringUTF(apn);
            jclass tClazz = (jclass)jenv->CallObjectMethod(classLoader,loadClass,param);
            if (ClearException(jenv)) {
                ALOG("JniHelper - Exception","No");
                continue;
            }
            return tClazz;
        }
    ALOG("JniHelper - Exception","No");
    return NULL;
}


// Module Methods 

jobject JniHelper::getGlobalContext(JNIEnv *env) {
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

void JniHelper::printJobject(JNIEnv *env, jobject obj) {
    jclass cls = env->GetObjectClass(obj);
    jmethodID toString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    jstring stringVal = (jstring)env->CallObjectMethod(obj, toString);
    LOGD("Hello - To String: %s\n", Jstring2CStr(env, stringVal));
}

jclass JniHelper::loadClassByPathClassLoader(JNIEnv *env, const char *className){
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

char* JniHelper::Jstring2CStr(JNIEnv* env, jstring jstr) {
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