#include "replicodehandler.h"

#include "replicodehighlighter.h"

#include <sstream>
#include <chrono>
#include <r_code/image.h>
#include <r_code/image_impl.h>
#include <r_exec/init.h>
#include <r_exec/opcodes.h>
#include <r_exec/object.h>
#include <r_exec/callbacks.h>           // for Callbacks
#include <r_comp/segments.h>
#include <r_exec/mem.h>
#include <r_comp/segments.h>
#include <r_comp/decompiler.h>
#include <QDebug>
#include <QSet>
#include <QFile>

ReplicodeHandler::ReplicodeHandler(QObject *parent) : QObject(parent),
    m_mem(nullptr),
    m_image(nullptr),
    m_metadata(nullptr)
{
    initialize();
}

ReplicodeHandler::~ReplicodeHandler()
{
    delete m_metadata;
    delete m_image;
}

void ReplicodeHandler::loadImage(QString file)
{
    if (!QFile::exists(file)) {
        emit error("Trying to open file that doesn't exist: " + file);
        return;
    }

    if (!m_image) {
        emit error("Replicode not initialized");
        return;
    }

    std::ifstream input(file.toStdString(), std::ios::binary | std::ios::in);
    r_code::Image<r_code::ImageImpl> *image;
    image = r_code::Image<r_code::ImageImpl>::Read(input);
    m_image->load(image);

    decompileImage(m_image);
}

void ReplicodeHandler::loadSource(QString file)
{
    if (!m_image) {
        emit error("Replicode not initialized");
        return;
    }


    std::string errorString;
    if (!r_exec::Compile(file.toLocal8Bit().constData(),
                         errorString,
                         m_image,
                         m_metadata,
                         false)) {
        emit error("Unable to compile " + file + ":\n" + QString::fromStdString(errorString));
        return;
    }
    decompileImage(m_image);

    if (m_mem) {
        delete m_mem;
    }

    m_mem = new r_exec::Mem<r_exec::LObject, r_exec::MemStatic>;

    r_code::vector<r_code::Code *> ram_objects;
    m_image->get_objects(m_mem, ram_objects);
    m_mem->metadata = m_metadata;
    m_mem->init(50000, // base period
                6, // reduction core count
                2, // time core count
                0.9, // model intertia sr thr
                6, // model intertia cnt thr
                0.1, // tpx dsr thr
                0, // min sim time horizon
                0, //max sim time horizon
                0.3, // sim time horizon
                500000, // tpx time horizon
                250000, // perf sampling period
                0.00001, // float tolerance
                10000, // time tolerance
                3600000, // primary thz
                7200000, // secondary thz
                true, // debug
                1, // ntf mk resilience
                1000, // goal pred success resilience
                2, // probe level
                0xCC // trace levels
                );

    uint64_t stdin_oid;
    std::string stdin_symbol("stdin");
    uint64_t stdout_oid;
    std::string stdout_symbol("stdout");
    uint64_t self_oid;
    std::string self_symbol("self");
    std::unordered_map<uintptr_t, std::string>::const_iterator n;

    for (const std::pair<const uint32_t, std::string> &symbol : m_image->object_names.symbols) {
        if (symbol.second == stdin_symbol) {
            stdin_oid = symbol.first;
        } else if (symbol.second == stdout_symbol) {
            stdout_oid = symbol.first;
        } else if (symbol.second == self_symbol) {
            self_oid = symbol.first;
        }
    }

    if (!m_mem->load(ram_objects.as_std(), stdin_oid, stdout_oid, self_oid)) {
        emit error("Memory failed to load objects");
        return;
    }
}

bool ReplicodeHandler::start()
{
    if (!m_mem) {
        return false;
    }

    uint64_t startTime = m_mem->start();

    return (startTime != 0);
}

void ReplicodeHandler::decompileImage(r_comp::Image *image)
{
    m_nodes.clear();
    m_edges.clear();

    r_comp::Decompiler decompiler;
    decompiler.init(m_metadata);

    uint64_t objectCount = decompiler.decompile_references(image);

    for (size_t i=0; i<objectCount; i++) {
        std::ostringstream source;
        source.precision(2);
        decompiler.decompile_object(i, &source, 0);
        QString nodeName = QString::fromStdString(decompiler.get_object_name(i));

        QString type = QString::fromStdString(m_metadata->classes_by_opcodes[image->code_segment.objects[i]->code[0].asOpcode()].str_opcode);

        QString group;
        if (type.startsWith("mk.")) {
            group = "passive";
        } else if (type.startsWith("ont")) {
            group = "passive";
        } else if (type.startsWith("ent")) {
            group = "passive";
        } else if (type.contains("fact")) {
            group = "passive";
        } else if (type.contains("mdl")) {
            group = "active";
        } else if (type.startsWith("cst")) {
            group = "passive";
        } else if (type.contains("pgm")) {
            group = "active";
        } else if (type.contains("grp")) {
            group = "groups";
        } else if (type.contains("perf")) {
            group = "passive";
        } else if (type.contains("cmd")) {
            group = "passive";
        } else {
            qDebug() << "Uncategorized object class" << nodeName << type;
            continue;
        }

        Node node;
        node.group = group;
        node.subgroup = type;

        node.displayName = nodeName;
        if (!nodeName.contains(type)) {
            node.displayName += " (" + type + ')';
        }

        QTextDocument *sourceDoc = new QTextDocument(QString::fromStdString(source.str()));
        new ReplicodeHighlighter(sourceDoc);
        node.sourcecode = std::shared_ptr<QTextDocument>(sourceDoc);
        m_nodes.insert(nodeName, node);

        r_code::SysObject *imageObject = image->code_segment.objects[i];
        for (size_t j=0; j<imageObject->views.size(); j++) {
            r_code::SysView *view = imageObject->views[j];
            for (size_t k=0; k<view->references.size(); k++) {
                Edge edge;
                edge.source = nodeName;
                edge.target = QString::fromStdString(decompiler.get_object_name(view->references[k]));
                edge.isView = true;
                m_edges.append(edge);
            }
        }
        for (size_t j=0; j<imageObject->references.size(); j++) {
            Edge edge;
            edge.source = nodeName;
            edge.target = QString::fromStdString(decompiler.get_object_name(imageObject->references[j]));
            m_edges.append(edge);
        }
    }
}

bool testCallback(uint64_t time, bool suspended, const char *msg, uint8_t object_count, r_code::Code **objects)
{
    std::cout << DebugStream::timestamp(time) << ": " << msg << (suspended ? " (suspended)" : "") << std::endl;

    for (uint8_t i = 0; i < object_count; ++i) {
        objects[i]->trace();
    }
    return true;
}

bool ReplicodeHandler::initialize()
{
    delete m_metadata;
    delete m_image;

    m_metadata = new r_comp::Metadata;
    m_image = new r_comp::Image;

    return r_exec::Init(nullptr,
                        []() -> uint64_t {
                            using namespace std::chrono;
                            return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
                        },
                        "user.classes.replicode",
                        m_image,
                        m_metadata
                        );

    r_exec::Callbacks::Register(std::string("test"), testCallback);
}

void ReplicodeHandler::stop()
{
    if (!m_mem) {
        return;
    }
    m_mem->stop();

    r_comp::Image *image = m_mem->get_objects();
    // Ensure that we get proper names
    image->object_names.symbols = m_image->object_names.symbols;
    decompileImage(image);
    delete image;
}
