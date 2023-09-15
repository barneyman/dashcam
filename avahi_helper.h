#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-glib/glib-watch.h>
#include <avahi-common/simple-watch.h>
#include <stdio.h>
#include <thread>
#include <string>
#include <vector>
#include <atomic>

class avahiHelper
{
protected:

    AvahiSimplePoll *m_simple_poll=NULL;
    const AvahiPoll *m_poll_api=NULL;
    AvahiClient *m_avahiClient=NULL;
    AvahiServiceBrowser *m_browser=NULL;

    std::thread *m_browserThread=NULL;
    std::string m_service;

    std::vector<std::pair<std::string,std::string>> m_servicesFound;

    bool avahi_finished()
    {
        return m_scanDone && m_servicesInFlux==0;   
    }

private:

    std::atomic<bool> m_scanDone;
    std::atomic<int> m_servicesInFlux;

public:

    avahiHelper(const char* service):
        m_service(service),
        m_servicesInFlux(0),m_scanDone(false)
    {
        // browse ..
        m_simple_poll = avahi_simple_poll_new();
        m_poll_api = avahi_simple_poll_get(m_simple_poll);

        m_browserThread=new std::thread(staticBrowse,this);
    }


    virtual ~avahiHelper()
    {
        // stop the loop
        avahi_simple_poll_quit(m_simple_poll);
        // join it
        m_browserThread->join();
        delete m_browserThread;

        // clean up
        avahi_service_browser_free(m_browser);
        avahi_client_free(m_avahiClient);
        avahi_simple_poll_free(m_simple_poll);
    }

    static void staticBrowse(avahiHelper *owner)
    {
        owner->Browse();
    }

    void Browse()
    {
        int avahiError=0;
        m_avahiClient=
            avahi_client_new(
                m_poll_api,
                AVAHI_CLIENT_NO_FAIL,
                staticAvahiClientCallback,
                this,
                &avahiError
            );

        const char *err=avahi_strerror(avahiError);

        m_browser=avahi_service_browser_new(
            m_avahiClient,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            //"_dashcam._tcp",NULL,
            //"_dashcam._tcp",NULL,
            m_service.c_str(),NULL,
            (AvahiLookupFlags)AVAHI_LOOKUP_USE_MULTICAST,
            staticAvahiBrowserCallback,
            this
        );

        avahi_simple_poll_loop(m_simple_poll);                
    }    



    static void staticAvahiClientCallback(AvahiClient *s,
        AvahiClientState state /**< The new state of the client */,
        void* userdata )
    {
    }

    static void staticAvahiBrowserCallback(AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags,
        void *userdata)
    {
        ((avahiHelper*)userdata)->avahiBrowserCallback(interface,protocol,event,name,type,domain,flags);
    }

    static void staticResolveCallback(AvahiServiceResolver *r,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiResolverEvent event,
        const char *name,
        const char *type,
        const char *domain,
        const char *host_name,
        const AvahiAddress *a,
        uint16_t port,
        AvahiStringList *txt,
        AvahiLookupResultFlags flags,
        void *userdata)
    {
        ((avahiHelper*)userdata)->resolveCallback(event,name,type,domain,host_name,a,port,txt,flags);
        avahi_service_resolver_free(r);
    }
    
protected:

    void avahiBrowserCallback( AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AvahiLookupResultFlags flags)
    {
        switch(event)
        {
            case AVAHI_BROWSER_NEW:
                m_servicesInFlux++;
                avahi_service_resolver_new( m_avahiClient, 
                                            interface, protocol, 
                                            name, type, domain, 
                                            AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, 
                                            staticResolveCallback, this);
                break;
            case AVAHI_BROWSER_REMOVE:
                printf("removed: %s\r\n",name);
                break;
            case AVAHI_BROWSER_ALL_FOR_NOW:
                m_scanDone=true;
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                break;
        }
    }

    void resolveCallback(AvahiResolverEvent event,
        const char *name,
        const char *type,
        const char *domain,
        const char *host_name,
        const AvahiAddress *address,
        uint16_t port,
        AvahiStringList *txt,
        AvahiLookupResultFlags flags)
    {
        char a[AVAHI_ADDRESS_STR_MAX], *t;
        if(address)
        {
            avahi_address_snprint(a, sizeof(a), address);

            if(address->proto==AVAHI_PROTO_INET)
            {
                printf("adding %s - %s\n\r",host_name, a);
                m_servicesFound.push_back(std::pair<std::string, std::string>(host_name,a));
            }
        }
        m_servicesInFlux--;
    }




};